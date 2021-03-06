/*
 * Copyright (C) 2017 Cogimon
 * Author: Enrico Mingo Hoffman
 * email:  enrico.mingo@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU Lesser General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#ifndef __BOUNDS_FORCE_WRENCHLIMITS_H__
#define __BOUNDS_FORCE_WRENCHLIMITS_H__

 #include <OpenSoT/Constraint.h>
#include <Eigen/Dense>

 namespace OpenSoT {
    namespace constraints {
        namespace force {
            class WrenchLimits: public Constraint<Eigen::MatrixXd, Eigen::VectorXd> {
            public:
                typedef boost::shared_ptr<WrenchLimits> Ptr;
            private:
                double _WrenchLimit;
            public:
                WrenchLimits(const double wrenchLimit,
                             const unsigned int x_size);

                double getWrenchLimits();

                void setWrenchLimits(const double wrenchLimit);


            private:
                void generateBounds();
            };
        }
    }
 }

#endif
