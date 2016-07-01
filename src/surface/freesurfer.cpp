/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "surface/freesurfer.h"

#include "debug.h"

namespace MR
{
  namespace Surface
  {
    namespace FreeSurfer
    {



      void read_annot (const std::string& path, label_vector_type& labels, Connectome::LUT& lut)
      {
        std::ifstream in (path.c_str(), std::ios_base::in | std::ios_base::binary);
        if (!in)
          throw Exception ("Error opening input file!");

        const int32_t num_vertices = get_BE<int32_t> (in);
        std::vector<int32_t> vertices, vertex_labels;
        vertices.reserve (num_vertices);
        vertex_labels.reserve (num_vertices);
        for (int32_t i = 0; i != num_vertices; ++i) {
          vertices.push_back (get_BE<int32_t> (in));
          vertex_labels.push_back (get_BE<int32_t> (in));
        }

        const int32_t colortable_present = get_BE<int32_t> (in);
        if (!in.good()) {
          WARN ("FreeSurfer annotation file \"" + Path::basename (path) + "\" does not contain colortable information");
          labels = label_vector_type::Zero (num_vertices);
          for (size_t i = 0; i != vertices.size(); ++i)
            labels[vertices[i]] = vertex_labels[i];
          return;
        }
        if (!colortable_present)
          throw Exception ("Error reading FreeSurfer annotation file \"" + Path::basename (path) + "\": Unexpected colortable flag");

        // Structure that will map from the colour-based structure identifier to a more sensible index
        std::map<int32_t, Connectome::node_t> rgb2index;

        int32_t num_entries = get_BE<int32_t> (in);
        if (num_entries > 0) {

          const int32_t orig_lut_name_length = get_BE<int32_t> (in);
          std::unique_ptr<char> orig_lut_name (new char[orig_lut_name_length]);
          in.read (orig_lut_name.get(), orig_lut_name_length);
          for (int32_t i = 0; i != num_entries; ++i) {
            const int32_t struct_name_length = get_BE<int32_t> (in);
            std::unique_ptr<char> struct_name (new char[struct_name_length]);
            in.read (struct_name.get(), struct_name_length);
            const int32_t r    = get_BE<int32_t> (in);
            const int32_t g    = get_BE<int32_t> (in);
            const int32_t b    = get_BE<int32_t> (in);
            const int32_t flag = get_BE<int32_t> (in);
            const int32_t id   = r + (g << 8) + (b << 16) + (flag << 24);
            lut.insert (std::make_pair (i, Connectome::LUT_node (std::string (struct_name.get()), r, g, b)));
            rgb2index[id] = i;
          }

        } else {

          const int32_t version = -num_entries;
          if (version != 2)
            throw Exception ("Error reading FreeSurfer annotation file \"" + Path::basename (path) + "\": Unsupported file version (" + str(version) + ")");

          num_entries = get_BE<int32_t> (in);
          const int32_t orig_lut_name_length = get_BE<int32_t> (in);
          std::unique_ptr<char> orig_lut_name (new char[orig_lut_name_length]);
          in.read (orig_lut_name.get(), orig_lut_name_length);

          const int32_t num_entries_to_read = get_BE<int32_t> (in);
          for (int32_t i = 0; i != num_entries_to_read; ++i) {
            const int32_t structure = get_BE<int32_t> (in) + 1;
            if (structure < 0)
              throw Exception ("Error reading FreeSurfer annotation file \"" + Path::basename (path) + "\": Negative structure index");
            if (lut.find (structure) != lut.end())
              throw Exception ("Error reading FreeSurfer annotation file \"" + Path::basename (path) + "\": Duplicate structure index");
            const int32_t struct_name_length = get_BE<int32_t> (in);
            std::unique_ptr<char> struct_name (new char[struct_name_length]);
            in.read (struct_name.get(), struct_name_length);
            const int32_t r    = get_BE<int32_t> (in);
            const int32_t g    = get_BE<int32_t> (in);
            const int32_t b    = get_BE<int32_t> (in);
            const int32_t flag = get_BE<int32_t> (in);
            const int32_t id   = r + (g << 8) + (b << 16) + (flag << 24);
            lut.insert (std::make_pair (structure, Connectome::LUT_node (std::string (struct_name.get()), r, g, b)));
            rgb2index[id] = structure;
          }

        }

        labels = label_vector_type::Zero (num_vertices);
        for (size_t i = 0; i != vertices.size(); ++i)
          labels[vertices[i]] = rgb2index[vertex_labels[i]];
      }



    }
  }
}