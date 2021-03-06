#include <OpenSoT/solvers/OSQPBackEnd.h>
#include <osqp/glob_opts.h>
#include <exception>
using namespace OpenSoT::solvers;

OSQPBackEnd::OSQPBackEnd(const int number_of_variables,
                         const int number_of_constraints,
                         const double eps_regularisation):
    BackEnd(number_of_variables, number_of_constraints),
    _eps_regularisation(eps_regularisation),
    _I(number_of_variables)
{
    
    #ifdef DLONG
        throw std::runtime_error("DLONG option in OSQP should be set to OFF in CMakeLists!");
    #endif
    
    _eye.setIdentity(number_of_variables, number_of_variables);
    _P_values.resize(getNumVariables()*(getNumVariables() + 1)/2);
    
    _settings.reset(new OSQPSettings());
     osqp_set_default_settings(_settings.get());
    _settings->verbose = 0;
    
   
    
    _data.reset(new OSQPData());
    _Pcsc.reset(new csc());
    _Acsc.reset(new csc());
       
    
}

void OSQPBackEnd::__generate_data_struct(const int number_of_variables, 
                                         const int number_of_constraints, 
                                         const int number_of_bounds)
{
    _lb_piled.setConstant(number_of_bounds + number_of_constraints, -1.0);
    _ub_piled.setConstant(number_of_bounds + number_of_constraints,  1.0);

    /* Set appropriate sparsity pattern to P (upper triangular) */
    
    Eigen::MatrixXd sp_pattern, ones;
    sp_pattern.setZero(getNumVariables(), getNumVariables());
    ones.setOnes(getNumVariables(), getNumVariables());
    
    sp_pattern.triangularView<Eigen::Upper>() = ones;
    
    _Psparse = sp_pattern.sparseView();
    _Psparse.makeCompressed();
    
    setCSCMatrix(_Pcsc.get(), _Psparse);
    _Pcsc->x = _P_values.data();


    /* Set appropriate sparsity pattern to A (dense + diagonal) */
    ones.setOnes(number_of_constraints, number_of_variables);
    sp_pattern.setZero(number_of_constraints + number_of_bounds, number_of_variables);
    sp_pattern.topRows(number_of_constraints) = ones;
    sp_pattern.bottomRows(number_of_bounds) = Eigen::MatrixXd::Identity(number_of_bounds, number_of_bounds);

    _Asparse = sp_pattern.sparseView();
    _Asparse.makeCompressed();

    int bounds_defined = number_of_bounds > 0 ? 1 : 0;
    _Adense.setOnes(number_of_constraints + bounds_defined, number_of_variables);

    setCSCMatrix(_Acsc.get(), _Asparse);
    _Acsc->x = _Adense.data();




    /* Fill data */

    _data->n = number_of_variables;
    _data->m = number_of_constraints + number_of_bounds;
    _data->l = _lb_piled.data();
    _data->u = _ub_piled.data();
    _data->q = _g.data();
    _data->A = _Acsc.get();
    _data->P = _Pcsc.get();

    
    
}

void OpenSoT::solvers::OSQPBackEnd::update_data_struct()
{
}


void OSQPBackEnd::setCSCMatrix(csc* a, Eigen::SparseMatrix<double>& A)
{
    a->m = A.rows();
    a->n = A.cols();
    a->nzmax = A.nonZeros();
    a->x = A.valuePtr();
    a->i = A.innerIndexPtr();
    a->p = A.outerIndexPtr();
}

bool OSQPBackEnd::updateTask(const Eigen::MatrixXd &H, const Eigen::VectorXd &g)
{
    bool success = BackEnd::updateTask(H, g);
    
    if(!success)
    {
        return false;
    }
    
    
    int idx = 0;
    for(int c = 0; c < getNumVariables(); c++)
    {
        _P_values.segment(idx, c+1) = _H.col(c).head(c+1);
        _P_values.segment(idx, c+1)(c) += _eps_regularisation;
        idx += c+1;
    }
    
    setCSCMatrix(_Pcsc.get(), _Psparse);
    _data->P->x = _P_values.data();
    _data->q = _g.data();
    
    return true;
}




bool OSQPBackEnd::updateConstraints(const Eigen::Ref<const Eigen::MatrixXd>& A, 
                                const Eigen::Ref<const Eigen::VectorXd>& lA, 
                                const Eigen::Ref<const Eigen::VectorXd>& uA)
{
    if(A.rows())
    {
        bool success = BackEnd::updateConstraints(A, lA, uA);

        if(!success)
        {
            return false;
        }
        

        /* Update values in A upper part (constraints) */
        _Adense.topRows(getNumConstraints()) = _A;
        setCSCMatrix(_Acsc.get(), _Asparse); // Asparse may be reallocated???
        _data->A->x = _Adense.data();
        
        /* Update constraints bounds */
        _lb_piled.head(getNumConstraints()) = _lA;
        _ub_piled.head(getNumConstraints()) = _uA;
        _data->l = _lb_piled.data();
        _data->u = _ub_piled.data();
    }
    
    return true;
}

bool OSQPBackEnd::updateBounds(const Eigen::VectorXd& l, const Eigen::VectorXd& u)
{
    if(l.rows() > 0)
    {
        bool success =  OpenSoT::solvers::BackEnd::updateBounds(l, u);

        if(!success)
        {
            return false;
        }

        _lb_piled.tail(getNumVariables()) = _l;
        _ub_piled.tail(getNumVariables()) = _u;
        _data->l = _lb_piled.data(); // lb_piled may be reallocated???
        _data->u = _ub_piled.data(); // ub_piled may be reallocated???
    }
    
    return true;
}


bool OSQPBackEnd::solve()
{
    
    
    osqp_update_lin_cost(_workspace.get(), _g.data());
    osqp_update_bounds(_workspace.get(), _lb_piled.data(), _ub_piled.data());
    osqp_update_A(_workspace.get(), _Adense.data(), nullptr, _Adense.size());
    osqp_update_P(_workspace.get(), _P_values.data(), nullptr, _P_values.size());
    
    
    
    int exitflag = osqp_solve(_workspace.get());
    
    _solution = Eigen::Map<Eigen::VectorXd>(_workspace->solution->x, _solution.size());
    
    return exitflag == 0;
    
}

boost::any OSQPBackEnd::getOptions()
{
    return _settings;
}

void OSQPBackEnd::setOptions(const boost::any &options)
{
    _settings.reset(new OSQPSettings(boost::any_cast<OSQPSettings>(options)));
    _workspace->settings = _settings.get();
}

bool OSQPBackEnd::initProblem(const Eigen::MatrixXd &H, const Eigen::VectorXd &g,
                                 const Eigen::MatrixXd &A,
                                 const Eigen::VectorXd &lA, const Eigen::VectorXd &uA,
                                 const Eigen::VectorXd &l, const Eigen::VectorXd &u)
{
    _H = H; _g = g; _A = A; _lA = lA; _uA = uA; _l = l; _u = u; //this is needed since updateX should be used just to update and not init (maybe can be done in the base class)
    __generate_data_struct(H.rows(), A.rows(), l.size());

    bool success = true;
    success = updateTask(H, g) && success;
    if(A.rows() > 0)
        success = updateConstraints(A, lA, uA) && success;
    if(l.rows() > 0)
        success = updateBounds(l, u) && success;
    
    if( ((_ub_piled - _lb_piled).array() < 0).any() )
    {
        XBot::Logger::error("OSQP: invalid bounds\n");
        return false;
    }
    
    _workspace.reset( osqp_setup(_data.get(), _settings.get()) );
    
    if(!_workspace)
    {
        XBot::Logger::error("OSQP: unable to setup workspace\n");
        return false;
    }
    
    success = solve() && success;
    
    return success;
}

double OSQPBackEnd::getObjective()
{
    return _workspace->info->obj_val;
}

//void OSQPBackEnd::print_csc_matrix_raw(csc* a, const std::string& name)
//{
//    XBot::Logger::info("%s->m: %i\n",name.c_str(), a->m);
//    XBot::Logger::info("%s->n: %i\n",name.c_str(), a->n);
//    XBot::Logger::info("%s->nzmax: %i\n",name.c_str(), a->nzmax);
//    XBot::Logger::info("%s->nz: %i\n",name.c_str(), a->nz);

//    XBot::Logger::info("%s->x:{",name.c_str());
//    for (int i=0; i< sizeof(a->x)/sizeof(a->x[0]); i++)
//        XBot::Logger::info("%f ",a->x[i]);
//    XBot::Logger::info("}\n");
//    XBot::Logger::info("%s->i:{",name.c_str());
//    for (int i=0; i< sizeof(a->i)/sizeof(a->i[0]); i++)
//        XBot::Logger::info("%f ",a->i[i]);
//    XBot::Logger::info("}\n");
//    XBot::Logger::info("%s->p:{",name.c_str());
//    for (int i=0; i< sizeof(a->p)/sizeof(a->p[0]); i++)
//        XBot::Logger::info("%f ",a->p[i]);
//    XBot::Logger::info("}\n");
//}

OSQPBackEnd::~OSQPBackEnd()
{

}
