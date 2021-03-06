/*
 * Copyright (C) 2014 Walkman
 * Author: Alessio Rocchi
 * email:  alessio.rocchi@iit.it
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

#ifndef __SOLVER_H__
#define __SOLVER_H__

#include <OpenSoT/Task.h>
#include <OpenSoT/Constraint.h>
#include <list>

using namespace std;

 namespace OpenSoT {
    template < class Matrix_type, class Vector_type >
    class Solver {
    public:
        typedef Task< Matrix_type, Vector_type > TaskType;
        typedef boost::shared_ptr<TaskType> TaskPtr;
        typedef Constraint< Matrix_type, Vector_type > ConstraintType;
        typedef boost::shared_ptr<ConstraintType> ConstraintPtr;
        typedef Solver< Matrix_type, Vector_type > SolverType;
        typedef boost::shared_ptr<SolverType> SolverPtr;
        typedef vector <TaskPtr> Stack;

    protected:
        vector <TaskPtr> _tasks;
        ConstraintPtr _bounds;
        ConstraintPtr _globalConstraints;

        /**
         * @brief _log implement this on the solver to log data
         * @param logger a pointer to a MatLogger
         */
        virtual void _log(XBot::MatLogger::Ptr logger)
        {

        }


    public:

        /**
         * @brief Solver an interface for a generic solver
         * @param stack a vector of pointers to tasks
         */
        Solver(vector <TaskPtr>& stack) : _tasks(stack){}

        /**
         * @brief Solver an interface for a generic solver
         * @param stack a vector of pointers to tasks
         * @param bounds a global bound for the problem
         */
        Solver(vector <TaskPtr>& stack,
               ConstraintPtr bounds) : _tasks(stack), _bounds(bounds) {}

        /**
         * @brief Solver an interface for a generic solver
         * @param stack a vector of pointers to tasks
         * @param bounds a global bound for the problem
         * @param globalConstraints a global constrains for the problem
         */
        Solver(vector<TaskPtr> &stack, ConstraintPtr bounds, ConstraintPtr globalConstraints):
            _tasks(stack), _bounds(bounds), _globalConstraints(globalConstraints) {}

        virtual ~Solver(){}

        /**
         * @brief solve solve an Optimization problem
         * @param solution the solution
         * @return  true if solved/solvable
         */
        virtual bool solve(Vector_type& solution) = 0;

        /**
         * @brief log logs data related to the solver
         * @param logger a pointer to a MatLogger
         */
        virtual void log(XBot::MatLogger::Ptr logger)
        {
            _log(logger);
        }
    };
 }

#endif
