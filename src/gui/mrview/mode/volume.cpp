/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "gui/opengl/lighting.h"
#include "math/vector.h"
#include "gui/mrview/mode/volume.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"
#include "gui/dialog/lighting.h"
#include "gui/mrview/tool/view.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        std::string Volume::Shader::vertex_shader_source (const Displayable&) 
        {
          std::string source = 
            "layout(location=0) in vec3 vertpos;\n"
            "uniform mat4 M;\n"
            "out vec3 texcoord;\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) 
            source += 
              "uniform mat4 overlay_M" + str(n) + ";\n"
              "out vec3 overlay_texcoord" + str(n) + ";\n";

          source += 
            "void main () {\n"
            "  texcoord = vertpos;\n"
            "  gl_Position =  M * vec4 (vertpos,1);\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) 
            source += 
              "  overlay_texcoord"+str(n) + " = (overlay_M"+str(n) + " * vec4 (vertpos,1)).xyz;\n";

          source +=
            "}\n";

          return source;
        }






        std::string Volume::Shader::fragment_shader_source (const Displayable& object)
        {

          std::vector< std::pair<GL::vec4,bool> > clip = mode.get_active_clip_planes();

          std::string source = object.declare_shader_variables() +
            "uniform sampler3D image_sampler;\n"
            "in vec3 texcoord;\n";

          for (size_t n = 0; n < clip.size(); ++n)
            source += 
              "uniform vec4 clip" + str(n) + ";\n"
              "uniform int clip" + str(n) + "_selected;\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) {
            source += mode.overlays_for_3D[n]->declare_shader_variables ("overlay"+str(n)+"_") +
              "uniform sampler3D overlay_sampler"+str(n) + ";\n"
              "uniform vec3 overlay_ray"+str(n) + ";\n"
              "in vec3 overlay_texcoord"+str(n) + ";\n";
          }

          source +=
            "uniform sampler2D depth_sampler;\n"
            "uniform mat4 M;\n"
            "uniform float ray_z, selection_thickness;\n"
            "uniform vec3 ray;\n"
            "out vec4 final_color;\n"
            "void main () {\n"
            "  float amplitude;\n"
            "  vec4 color;\n";


          source += 
            "  final_color = vec4 (0.0);\n"
            "  float dither = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453);\n"
            "  vec3 coord = texcoord + ray * dither;\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) 
            source += 
              "  vec3 overlay_coord"+str(n) +" = overlay_texcoord"+str(n) + " + overlay_ray"+str(n) + " * dither;\n";

              source +=
            "  float depth = texelFetch (depth_sampler, ivec2(gl_FragCoord.xy), 0).r;\n"
            "  float current_depth = gl_FragCoord.z + ray_z * dither;\n"
            "  int nmax = 10000;\n"
            "  if (ray.x < 0.0) nmax = int (-texcoord.s/ray.x);\n"
            "  else if (ray.x > 0.0) nmax = int ((1.0-texcoord.s) / ray.x);\n"
            "  if (ray.y < 0.0) nmax = min (nmax, int (-texcoord.t/ray.y));\n"
            "  else if (ray.y > 0.0) nmax = min (nmax, int ((1.0-texcoord.t) / ray.y));\n"
            "  if (ray.z < 0.0) nmax = min (nmax, int (-texcoord.p/ray.z));\n"
            "  else if (ray.z > 0.0) nmax = min (nmax, int ((1.0-texcoord.p) / ray.z));\n"
            "  nmax = min (nmax, int ((depth - current_depth) / ray_z));\n"
            "  if (nmax <= 0) return;\n"
            "  for (int n = 0; n < nmax; ++n) {\n"
            "    coord += ray;\n";

              if (clip.size()) {
                source += "    bool show = true;\n";
                for (size_t n = 0; n < clip.size(); ++n)
                  source += "    if (dot (coord, clip" + str(n) + ".xyz) > clip" + str(n) + ".w)\n";
                source += 
                  "          show = false;\n"
                  "    if (show) {\n";
              }


          source += 
            "      color = texture (image_sampler, coord);\n"
            "      amplitude = " + std::string (ColourMap::maps[object.colourmap].amplitude) + ";\n"
            "      if (!isnan(amplitude) && !isinf(amplitude)";

          if (object.use_discard_lower())
            source += " && amplitude >= lower";

          if (object.use_discard_upper())
            source += " && amplitude <= upper";

          source += " && amplitude >= alpha_offset) {\n"
            "        color.a = clamp ((amplitude - alpha_offset) * alpha_scale, 0, alpha);\n";

          if (!ColourMap::maps[object.colourmap].special) {
            source += 
              "        amplitude = clamp (";
            if (object.scale_inverted()) 
              source += "1.0 -";
            source += 
              " scale * (amplitude - offset), 0.0, 1.0);\n";
          }

          source += 
            std::string ("        ") + ColourMap::maps[object.colourmap].mapping;

          for (size_t n = 0; n < clip.size(); ++n) 
            source += 
              "        if (clip"+str(n)+"_selected != 0) {\n" 
              "          float dist = dot (coord, clip" + str(n) + ".xyz) - clip" + str(n) + ".w;\n"
              "          color.rgb = mix (vec3(1.5,0.0,0.0), color.rgb, clamp (abs(dist)/selection_thickness, 0.0, 1.0));\n"
              "        }\n";

          source +=
            "        final_color.rgb += (1.0 - final_color.a) * color.rgb * color.a;\n"
            "        final_color.a += color.a;\n"
            "      }\n";


          if (clip.size())
            source += "    }\n";




          // OVERLAYS:
          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) {
            source += 
              "    overlay_coord"+str(n) + " += overlay_ray"+str(n) + ";\n"
              "    if (overlay_coord"+str(n) + ".s >= 0.0 && overlay_coord"+str(n) + ".s <= 1.0 &&\n"
              "        overlay_coord"+str(n) + ".t >= 0.0 && overlay_coord"+str(n) + ".t <= 1.0 &&\n"
              "        overlay_coord"+str(n) + ".p >= 0.0 && overlay_coord"+str(n) + ".p <= 1.0) {\n"
              "      color = texture (overlay_sampler"+str(n) +", overlay_coord"+str(n) +");\n"
              "      amplitude = " + std::string (ColourMap::maps[mode.overlays_for_3D[n]->colourmap].amplitude) + ";\n"
              "      if (!isnan(amplitude) && !isinf(amplitude)";

            if (mode.overlays_for_3D[n]->use_discard_lower())
              source += " && amplitude >= overlay"+str(n)+"_lower";

            if (mode.overlays_for_3D[n]->use_discard_upper())
              source += " && amplitude <= overlay"+str(n)+"_upper";

            source += ") {\n";

            if (!ColourMap::maps[mode.overlays_for_3D[n]->colourmap].special) {
              source += 
                "        amplitude = clamp (";
              if (mode.overlays_for_3D[n]->scale_inverted()) 
                source += "1.0 -";
              source += 
                " overlay"+str(n)+"_scale * (amplitude - overlay"+str(n)+"_offset), 0.0, 1.0);\n";
            }

            source += 
              std::string ("        ") + ColourMap::maps[mode.overlays_for_3D[n]->colourmap].mapping +
              "        color.a = amplitude * overlay"+str(n) + "_alpha;\n"
              "        final_color.rgb += (1.0 - final_color.a) * color.rgb * color.a;\n"
              "        final_color.a += color.a;\n"
              "      }\n"
              "    }\n";
          }



          source += 
            "    if (final_color.a > 0.95) break;\n"
            "  }\n"
            "}\n";

          return source;
        }




        bool Volume::Shader::need_update (const Displayable& object) const 
        {
          if (mode.update_overlays) 
            return true;
          if (mode.get_active_clip_planes().size() != active_clip_planes) 
            return true;
          return Displayable::Shader::need_update (object);
        }



        void Volume::Shader::update (const Displayable& object) 
        {
          active_clip_planes = mode.get_active_clip_planes().size();
          Displayable::Shader::update (object);
        }





        namespace {

          inline GL::vec4 clip_real2tex (const GL::mat4& T2S, const GL::mat4& S2T, const GL::vec4& plane) 
          {
            GL::vec4 normal = T2S * GL::vec4 (plane[0], plane[1], plane[2], 0.0);
            GL::vec4 on_plane = S2T * GL::vec4 (plane[3]*plane[0], plane[3]*plane[1], plane[3]*plane[2], 1.0);
            normal[3] = on_plane[0]*normal[0] + on_plane[1]*normal[1] + on_plane[2]*normal[2];
            return normal;
          }


          inline GL::mat4 get_tex_to_scanner_matrix (const Image& image)
          {
            Point<> pos = image.interp.voxel2scanner (Point<> (-0.5f, -0.5f, -0.5f));
            Point<> vec_X = image.interp.voxel2scanner_dir (Point<> (image.interp.dim(0), 0.0f, 0.0f));
            Point<> vec_Y = image.interp.voxel2scanner_dir (Point<> (0.0f, image.interp.dim(1), 0.0f));
            Point<> vec_Z = image.interp.voxel2scanner_dir (Point<> (0.0f, 0.0f, image.interp.dim(2)));
            GL::mat4 T2S;
            T2S(0,0) = vec_X[0];
            T2S(1,0) = vec_X[1];
            T2S(2,0) = vec_X[2];

            T2S(0,1) = vec_Y[0];
            T2S(1,1) = vec_Y[1];
            T2S(2,1) = vec_Y[2];

            T2S(0,2) = vec_Z[0];
            T2S(1,2) = vec_Z[1];
            T2S(2,2) = vec_Z[2];

            T2S(0,3) = pos[0];
            T2S(1,3) = pos[1];
            T2S(2,3) = pos[2];

            T2S(3,0) = T2S(3,1) = T2S(3,2) = 0.0f; 
            T2S(3,3) = 1.0f;

            return T2S;
          }

        }









        void Volume::paint (Projection& projection)
        {
          // info for projection:
          int w = width(), h = height();
          float fov = FOV() / (float) (w+h);

          float depth = std::max (image()->interp.dim(0)*image()->interp.vox(0),
              std::max (image()->interp.dim(1)*image()->interp.vox(1),
                image()->interp.dim(2)*image()->interp.vox(2)));


          Math::Versor<float> Q = orientation();
          if (!Q) {
            Q = Math::Versor<float> (1.0, 0.0, 0.0, 0.0);
            set_orientation (Q);
          }

          // set up projection & modelview matrices:
          GL::mat4 P = GL::ortho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);
          GL::mat4 MV = adjust_projection_matrix (Q) * GL::translate  (-target()[0], -target()[1], -target()[2]);
          projection.set (MV, P);



          overlays_for_3D.clear();
          render_tools (projection, true);
          gl::Disable (gl::BLEND);
          gl::Enable (gl::DEPTH_TEST);
          gl::DepthMask (gl::TRUE_);

          draw_crosshairs (projection);




          GL::mat4 T2S = get_tex_to_scanner_matrix (*image());
          GL::mat4 M = projection.modelview_projection() * T2S;
          GL::mat4 S2T = GL::inv (T2S);

          int min_vox_index;
          if (image()->interp.vox(0) < image()->interp.vox (1)) 
            min_vox_index = image()->interp.vox(0) < image()->interp.vox (2) ? 0 : 2;
          else 
            min_vox_index = image()->interp.vox(1) < image()->interp.vox (2) ? 1 : 2;
          float step_size = 0.5 * image()->interp.vox(min_vox_index);
          Point<> ray = image()->interp.scanner2voxel_dir (projection.screen_normal() * step_size);
          ray[0] /= image()->interp.dim(0);
          ray[1] /= image()->interp.dim(1);
          ray[2] /= image()->interp.dim(2);



          if (!volume_VB || !volume_VAO || !volume_VI) {
            volume_VB.gen();
            volume_VI.gen();
            volume_VAO.gen();

            volume_VAO.bind();
            volume_VB.bind (gl::ARRAY_BUFFER);
            volume_VI.bind (gl::ELEMENT_ARRAY_BUFFER);

            gl::EnableVertexAttribArray (0);
            gl::VertexAttribPointer (0, 3, gl::BYTE, gl::FALSE_, 0, (void*)0);

            GLbyte vertices[] = {
              0, 0, 0,
              0, 0, 1,
              0, 1, 0,
              0, 1, 1,
              1, 0, 0,
              1, 0, 1,
              1, 1, 0,
              1, 1, 1
            };
            gl::BufferData (gl::ARRAY_BUFFER, sizeof(vertices), vertices, gl::STATIC_DRAW);
          }
          else {
            volume_VAO.bind();
            volume_VI.bind (gl::ELEMENT_ARRAY_BUFFER);
          }

          GLubyte indices[12];

          if (ray[0] < 0) {
            indices[0] = 4; 
            indices[1] = 5;
            indices[2] = 7;
            indices[3] = 6;
          }
          else {
            indices[0] = 0; 
            indices[1] = 1;
            indices[2] = 3;
            indices[3] = 2;
          }

          if (ray[1] < 0) {
            indices[4] = 2; 
            indices[5] = 3;
            indices[6] = 7;
            indices[7] = 6;
          }
          else {
            indices[4] = 0; 
            indices[5] = 1;
            indices[6] = 5;
            indices[7] = 4;
          }

          if (ray[2] < 0) {
            indices[8] = 1; 
            indices[9] = 3;
            indices[10] = 7;
            indices[11] = 5;
          }
          else {
            indices[8] = 0; 
            indices[9] = 2;
            indices[10] = 6;
            indices[11] = 4;
          }

          gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, gl::STREAM_DRAW);

          image()->update_texture3D();
          image()->set_use_transparency (true);

          image()->start (volume_shader, image()->scale_factor());
          gl::UniformMatrix4fv (gl::GetUniformLocation (volume_shader, "M"), 1, gl::FALSE_, M);
          gl::Uniform3fv (gl::GetUniformLocation (volume_shader, "ray"), 1, ray);
          gl::Uniform1i (gl::GetUniformLocation (volume_shader, "image_sampler"), 0);
          gl::Uniform1f (gl::GetUniformLocation (volume_shader, "selection_thickness"), 3.0*step_size);

          gl::ActiveTexture (gl::TEXTURE0);
          gl::BindTexture (gl::TEXTURE_3D, image()->texture());

          gl::ActiveTexture (gl::TEXTURE1);
          if (!depth_texture) {
            depth_texture.gen (gl::TEXTURE_2D);
            depth_texture.bind();
            depth_texture.set_interp (gl::NEAREST);
          }
          else 
            depth_texture.bind();

          gl::ReadBuffer (gl::BACK);
#if QT_VERSION >= 0x050100
          int m = window.windowHandle()->devicePixelRatio();
          gl::CopyTexImage2D (gl::TEXTURE_2D, 0, gl::DEPTH_COMPONENT, 0, 0, m*projection.width(), m*projection.height(), 0);
#else
          gl::CopyTexImage2D (gl::TEXTURE_2D, 0, gl::DEPTH_COMPONENT, 0, 0, projection.width(), projection.height(), 0);
#endif

          gl::Uniform1i (gl::GetUniformLocation (volume_shader, "depth_sampler"), 1);

          std::vector< std::pair<GL::vec4,bool> > clip = get_active_clip_planes();

          for (size_t n = 0; n < clip.size(); ++n) {
            gl::Uniform4fv (gl::GetUniformLocation (volume_shader, ("clip"+str(n)).c_str()), 1,
                clip_real2tex (T2S, S2T, clip[n].first));
            gl::Uniform1i (gl::GetUniformLocation (volume_shader, ("clip"+str(n)+"_selected").c_str()), clip[n].second);
          }

          for (int n = 0; n < overlays_for_3D.size(); ++n) {
            gl::ActiveTexture (gl::TEXTURE2 + n);
            gl::BindTexture (gl::TEXTURE_3D, overlays_for_3D[n]->texture());
            overlays_for_3D[n]->update_texture3D();
            overlays_for_3D[n]->texture().set_interp_on (overlays_for_3D[n]->interpolate());
            gl::Uniform1i (gl::GetUniformLocation (volume_shader, ("overlay_sampler"+str(n)).c_str()), 2+n);

            GL::mat4 overlay_M = GL::inv (get_tex_to_scanner_matrix (*overlays_for_3D[n])) * T2S;
            GL::vec4 overlay_ray = overlay_M * GL::vec4 (ray, 0.0);
            gl::UniformMatrix4fv (gl::GetUniformLocation (volume_shader, ("overlay_M"+str(n)).c_str()), 1, gl::FALSE_, overlay_M);
            gl::Uniform3fv (gl::GetUniformLocation (volume_shader, ("overlay_ray"+str(n)).c_str()), 1, overlay_ray);

            overlays_for_3D[n]->set_shader_variables (volume_shader, overlays_for_3D[n]->scale_factor(), "overlay"+str(n)+"_");
          }

          GL::vec4 ray_eye = M * GL::vec4 (ray, 0.0);
          gl::Uniform1f (gl::GetUniformLocation (volume_shader, "ray_z"), 0.5*ray_eye[2]);

          gl::Enable (gl::BLEND);
          gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);

          gl::DepthMask (gl::FALSE_);
          gl::ActiveTexture (gl::TEXTURE0);

          const GLsizei counts[] = { 4, 4, 4 };
          const GLvoid* starts[] = { reinterpret_cast<void*>(0), reinterpret_cast<void*>(4*sizeof(GLubyte)), reinterpret_cast<void*>(8*sizeof(GLubyte)) };

          gl::MultiDrawElements (gl::TRIANGLE_FAN, counts, gl::UNSIGNED_BYTE, starts, 3);
          image()->stop (volume_shader);

          gl::Disable (gl::BLEND);

          draw_orientation_labels (projection);
        }


        inline Tool::View* Volume::get_view_tool () const
        {
          Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(window.tools()->actions()[0])->dock;
          if (!dock) 
            return NULL;
          return dynamic_cast<Tool::View*> (dock->tool);
        }


        inline std::vector< std::pair<GL::vec4,bool> > Volume::get_active_clip_planes () const 
        {
          Tool::View* view = get_view_tool();
          return view ? view->get_active_clip_planes() : std::vector< std::pair<GL::vec4,bool> >();
        }



        inline std::vector<GL::vec4*> Volume::get_clip_planes_to_be_edited () const
        {
          Tool::View* view = get_view_tool();
          return view ? view->get_clip_planes_to_be_edited() : std::vector<GL::vec4*>();
        }



        inline void Volume::move_clip_planes_in_out (std::vector<GL::vec4*>& clip, float distance)
        {
          Point<> d = get_current_projection()->screen_normal();
          for (size_t n = 0; n < clip.size(); ++n) {
            GL::vec4& p (*clip[n]);
            p[3] += distance * (p[0]*d[0] + p[1]*d[1] + p[2]*d[2]);
          }
          updateGL();
        }


        inline void Volume::rotate_clip_planes (std::vector<GL::vec4*>& clip, const Math::Versor<float>& rot)
        {
          for (size_t n = 0; n < clip.size(); ++n) {
            GL::vec4& p (*clip[n]);
            float distance_to_focus = p[0]*focus()[0] + p[1]*focus()[1] + p[2]*focus()[2] - p[3];
            Math::Versor<float> norm (0.0f, p[0], p[1], p[2]);
            Math::Versor<float> rotated = norm * rot;
            rotated.normalise();
            p[0] = rotated[1];
            p[1] = rotated[2];
            p[2] = rotated[3];
            p[3] = p[0]*focus()[0] + p[1]*focus()[1] + p[2]*focus()[2] - distance_to_focus;
          }
          updateGL();
        }





        void Volume::slice_move_event (int x) 
        {
         
          std::vector<GL::vec4*> clip = get_clip_planes_to_be_edited();
          if (clip.size()) 
            move_clip_planes_in_out (clip, x * std::min (std::min (image()->header().vox(0), image()->header().vox(1)), image()->header().vox(2)));
          else
            Base::slice_move_event (x);
        }



        void Volume::pan_event () 
        {
          std::vector<GL::vec4*> clip = get_clip_planes_to_be_edited();
          if (clip.size()) {
            Point<> move = get_current_projection()->screen_to_model_direction (window.mouse_displacement(), target());
            for (size_t n = 0; n < clip.size(); ++n) {
              GL::vec4& p (*clip[n]);
              p[3] += (p[0]*move[0] + p[1]*move[1] + p[2]*move[2]);
            }
            updateGL();
          }
          else 
            Base::pan_event();
        }


        void Volume::panthrough_event () 
        {
          std::vector<GL::vec4*> clip = get_clip_planes_to_be_edited();
          if (clip.size()) 
            move_clip_planes_in_out (clip, MOVE_IN_OUT_FOV_MULTIPLIER * window.mouse_displacement().y() * FOV());
          else
            Base::panthrough_event();
        }



        void Volume::tilt_event () 
        {
          std::vector<GL::vec4*> clip = get_clip_planes_to_be_edited();
          if (clip.size()) {
            Math::Versor<float> rot = get_tilt_rotation();
            if (!rot) 
              return;
            rotate_clip_planes (clip, rot);
          }
          else 
            Base::tilt_event();
        }



        void Volume::rotate_event () 
        {
          std::vector<GL::vec4*> clip = get_clip_planes_to_be_edited();
          if (clip.size()) {
            Math::Versor<float> rot = get_rotate_rotation();
            if (!rot) 
              return;
            rotate_clip_planes (clip, rot);
          }
          else 
            Base::rotate_event();
        }


      }
    }
  }
}



