/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/10/09.

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

#ifndef __dwi_tractography_method_h__
#define __dwi_tractography_method_h__

#include "point.h"
#include "math/rng.h"
#include "image/voxel.h"
#include "image/interp/linear.h"
#include "dwi/tractography/shared.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

        class MethodBase {
          public:
            MethodBase (const SharedBase& shared) :
              pos (0.0, 0.0, 0.0),
              dir (0.0, 0.0, 1.0),
              values (shared.source_buffer.dim(3)),
              step_size (shared.step_size) { }

            MethodBase (const MethodBase& base) : 
              rng (base.rng),
              pos (0.0, 0.0, 0.0),
              dir (0.0, 0.0, 1.0),
              values (base.values.size()),
              step_size (base.step_size) { }

            Math::RNG rng;
            Point<value_type> pos, dir;
            std::vector<value_type> values;
            value_type step_size;

            template <class InterpolatorType> 
              inline bool get_data (InterpolatorType& source, const Point<value_type>& position) 
              {
                source.scanner (position); 
                if (!source) return (false);
                for (source[3] = 0; source[3] < source.dim(3); ++source[3])
                  values[source[3]] = source.value();
                return (!isnan (values[0]));
              }

            template <class InterpolatorType> 
              inline bool get_data (InterpolatorType& source) { 
                return (get_data (source, pos)); 
              }

            Point<value_type> random_direction (value_type max_angle, value_type sin_max_angle)
            {
              value_type phi = 2.0 * M_PI * rng.uniform();
              value_type theta;
              do { 
                theta = max_angle * rng.uniform();
              } while (sin_max_angle * rng.uniform() > sin (theta)); 
              return (Point<value_type> (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)));
            }


            Point<value_type> rotate_direction (const Point<value_type>& reference, const Point<value_type>& direction) 
            {
              using namespace Math;

              value_type n = sqrt (pow2(reference[0]) + pow2(reference[1]));
              if (n == 0.0)
                return (reference[2] < 0.0 ? -direction : direction);

              Point<value_type> m (reference[0]/n, reference[1]/n, 0.0);
              Point<value_type> mp (reference[2]*m[0], reference[2]*m[1], -n);

              value_type alpha = direction[2];
              value_type beta = direction[0]*m[0] + direction[1]*m[1];

              return (Point<value_type> (
                    direction[0] + alpha * reference[0] + beta * (mp[0] - m[0]),
                    direction[1] + alpha * reference[1] + beta * (mp[1] - m[1]),
                    direction[2] + alpha * (reference[2]-1.0) + beta * (mp[2] - m[2])
                    ));
            }


            Point<value_type> random_direction (const Point<value_type>& d, value_type max_angle, value_type sin_max_angle)
            {
              return (rotate_direction (d, random_direction (max_angle, sin_max_angle)));
            }


            virtual void reverse_track() { }
            virtual bool next() = 0;


        };



    }
  }
}

#endif



