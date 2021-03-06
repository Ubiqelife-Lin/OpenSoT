#include <gtest/gtest.h>
#include <OpenSoT/constraints/velocity/CartesianPositionConstraint.h>
#include <OpenSoT/utils/DefaultHumanoidStack.h>
#include <advr_humanoids_common_utils/idynutils.h>
#include <idynutils/tests_utils.h>
#include <iCub/iDynTree/yarp_kdl.h>
#include <yarp/sig/Vector.h>
#include <yarp/math/Math.h>
#include <yarp/math/SVD.h>
#include <cmath>
#include <ModelInterfaceIDYNUTILS/ModelInterfaceIDYNUTILS.h>
#include <advr_humanoids_common_utils/conversion_utils_YARP.h>
#define  s                1.0
#define  dT               0.001* s
#define  m_s              1.0
#define toRad(X) (X * M_PI/180.0)

using namespace OpenSoT::constraints::velocity;
using namespace yarp::math;

namespace {

// The fixture for testing class CartesianPositionConstraint.
class testCartesianPositionConstraint : public ::testing::Test{
public:
    typedef idynutils2 iDynUtils;
    static void null_deleter(iDynUtils *) {}
 protected:

  // You can remove any or all of the following functions if its body
  // is empty.

  testCartesianPositionConstraint() :
      coman("coman",
            std::string(OPENSOT_TESTS_ROBOTS_DIR)+"coman/coman.urdf",
            std::string(OPENSOT_TESTS_ROBOTS_DIR)+"coman/coman.srdf"),
      zeros(coman.getJointNames().size(),0.0),
      _DHS(coman, 3e-3, conversion_utils_YARP::toEigen(zeros)),
      _A(1,3),
      _b(1,-0.5)
  {
      std::string robotology_root = std::getenv("ROBOTOLOGY_ROOT");
      std::string relative_path = "/external/OpenSoT/tests/configs/coman/configs/config_coman.yaml";

      _path_to_cfg = robotology_root + relative_path;

      _model_ptr = std::dynamic_pointer_cast<XBot::ModelInterfaceIDYNUTILS>
              (XBot::ModelInterface::getModel(_path_to_cfg));
      _model_ptr->loadModel(boost::shared_ptr<iDynUtils>(&coman, &null_deleter));

      if(_model_ptr)
          std::cout<<"pointer address: "<<_model_ptr.get()<<std::endl;
      else
          std::cout<<"pointer is NULL "<<_model_ptr.get()<<std::endl;

    // You can do set-up work for each test here.


      // A,b represent a plane with normal along the z axis. We are imposing the z coordinate of the right_arm
      // should be greater than 0.5
      _A.zero(); _A(0,2) = -1.0;
      _cartesianPositionConstraint = new CartesianPositionConstraint(
                  conversion_utils_YARP::toEigen(zeros), _DHS.leftArm,
                                            conversion_utils_YARP::toEigen(_A),
                                            conversion_utils_YARP::toEigen(_b));
  }

  virtual ~testCartesianPositionConstraint() {
    // You can do clean-up work that doesn't throw exceptions here.
      if(_cartesianPositionConstraint != NULL) {
        delete _cartesianPositionConstraint;
        _cartesianPositionConstraint = NULL;
      }
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).
      _cartesianPositionConstraint->update(conversion_utils_YARP::toEigen(zeros));
      coman.updateiDynTreeModel(conversion_utils_YARP::toEigen(zeros),true);
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for CartesianPositionConstraint.

  iDynUtils coman;
  yarp::sig::Vector zeros;

  CartesianPositionConstraint* _cartesianPositionConstraint;
  OpenSoT::DefaultHumanoidStack _DHS;

  yarp::sig::Vector q;
  yarp::sig::Matrix _A;
  yarp::sig::Vector _b;
  XBot::ModelInterfaceIDYNUTILS::Ptr _model_ptr;
  std::string _path_to_cfg;
};

yarp::sig::Vector getGoodInitialPosition(iDynUtils& model) {
    yarp::sig::Vector q(model.iDyn3_model.getNrOfDOFs(), 0.0);
    yarp::sig::Vector leg(model.left_leg.getNrOfDOFs(), 0.0);
    leg[0] = -25.0 * M_PI/180.0;
    leg[3] =  50.0 * M_PI/180.0;
    leg[5] = -25.0 * M_PI/180.0;
    model.fromRobotToIDyn(leg, q, model.left_leg);
    model.fromRobotToIDyn(leg, q, model.right_leg);
    yarp::sig::Vector arm(model.left_arm.getNrOfDOFs(), 0.0);
    arm[0] = 20.0 * M_PI/180.0;
    arm[1] = 10.0 * M_PI/180.0;
    arm[3] = -80.0 * M_PI/180.0;
    model.fromRobotToIDyn(arm, q, model.left_arm);
    arm[1] = -arm[1];
    model.fromRobotToIDyn(arm, q, model.right_arm);
    return q;
}

TEST_F(testCartesianPositionConstraint, checkBoundsScaling) {
    // ------- Set The robot in a certain configuration ---------
    double bound = _cartesianPositionConstraint->getbUpperBound()[0];

    if(_cartesianPositionConstraint != NULL) {
      delete _cartesianPositionConstraint;
      _cartesianPositionConstraint = NULL;
    }
    _cartesianPositionConstraint = new CartesianPositionConstraint(
                conversion_utils_YARP::toEigen(zeros), _DHS.leftArm,
                conversion_utils_YARP::toEigen(_A),
                conversion_utils_YARP::toEigen(_b), 0.5);

    double boundSmall = _cartesianPositionConstraint->getbUpperBound()[0];


    EXPECT_DOUBLE_EQ(boundSmall,bound/2.0);
}

TEST_F(testCartesianPositionConstraint, sizesAreCorrect) {

    EXPECT_EQ(0, _cartesianPositionConstraint->getLowerBound().size()) << "lowerBound should have size 0"
                                                      << "but has size"
                                                      <<  _cartesianPositionConstraint->getLowerBound().size();
    EXPECT_EQ(0, _cartesianPositionConstraint->getUpperBound().size()) << "upperBound should have size 0"
                                                      << "but has size"
                                                      << _cartesianPositionConstraint->getUpperBound().size();

    EXPECT_EQ(0, _cartesianPositionConstraint->getAeq().rows()) << "Aeq should have size 0"
                                               << "but has size"
                                               << _cartesianPositionConstraint->getAeq().rows();

    EXPECT_EQ(0, _cartesianPositionConstraint->getbeq().size()) << "beq should have size 0"
                                               << "but has size"
                                               <<  _cartesianPositionConstraint->getbeq().size();


    EXPECT_EQ(coman.iDyn3_model.getNrOfDOFs(),_cartesianPositionConstraint->getAineq().cols()) <<  " Aineq should have number of columns equal to "
                                                                              << coman.iDyn3_model.getNrOfDOFs()
                                                                              << " but has has "
                                                                              << _cartesianPositionConstraint->getAeq().cols()
                                                                              << " columns instead";

    EXPECT_EQ(0,_cartesianPositionConstraint->getbLowerBound().size()) << "bLowerBound should have size 0"
                                                      << "but has size"
                                                      << _cartesianPositionConstraint->getbLowerBound().size();




    EXPECT_EQ(1,_cartesianPositionConstraint->getAineq().rows()) << "Aineq should have size "
                                                                 << 1
                                                                 << " but has size"
                                                                 << _cartesianPositionConstraint->getAineq().rows();


    EXPECT_EQ(1,_cartesianPositionConstraint->getbUpperBound().size()) << "bUpperBound should have size "
                                                                       << 1
                                                                       << " but has size"
                                                                       << _cartesianPositionConstraint->getbUpperBound().size();
}

// Tests that the Foo::getLowerBounds() are zero at the bounds
TEST_F(testCartesianPositionConstraint, BoundsAreCorrect) {
    // left arm z coordinate is at .52 m, the bound implies z > .50 m
    _DHS.leftArm->setLambda(0.3);
    q = getGoodInitialPosition(coman);
    coman.updateiDyn3Model(q, true);
    _DHS.leftArm->update(conversion_utils_YARP::toEigen(q));
    _cartesianPositionConstraint->update(conversion_utils_YARP::toEigen(q));

    Eigen::MatrixXd p = _DHS.leftArm->getActualPose();
    p(2,3) = 0.5;
    _DHS.leftArm->setReference(p);

    double e = 0.0;
    unsigned int i = 0;

    Eigen::MatrixXd tmp = -_DHS.leftArm->getA();
    EXPECT_TRUE(
                conversion_utils_YARP::fromEigentoYarp(_cartesianPositionConstraint->getAineq()) ==
                conversion_utils_YARP::fromEigentoYarp(tmp).submatrix(2,2,0,28))
        << "Aineq is \n"            <<
           conversion_utils_YARP::fromEigentoYarp(_cartesianPositionConstraint->getAineq()).toString()
        << "\n while J(2,:) is \n"  << conversion_utils_YARP::fromEigentoYarp(tmp).submatrix(2,2,0,28).toString();

                                                                                                              ;
    // when above the position bound, z_dot > z_dot_limit = -bUpperBound
    // and we expect z_dot_limit to be NEGATIVE (we can go up, but also down)
    EXPECT_LT(-_cartesianPositionConstraint->getbUpperBound()(0),0.0);
    do {
        std::cout<<"yarp::norm "<<norm(conversion_utils_YARP::fromEigentoYarp(_DHS.leftArm->getb()))<<std::endl;
        std::cout<<"squared norm "<<sqrt(_DHS.leftArm->getb().squaredNorm())<<std::endl;

        e = norm(conversion_utils_YARP::fromEigentoYarp(_DHS.leftArm->getb()));
        double previous_bUpperBound = -_cartesianPositionConstraint->getbUpperBound()(0);
        q += pinv(conversion_utils_YARP::fromEigentoYarp(_DHS.leftArm->getA()),1E-7)*
                conversion_utils_YARP::fromEigentoYarp(_DHS.leftArm->getb());
        coman.updateiDyn3Model(q, true);
        _DHS.leftArm->update(conversion_utils_YARP::toEigen(q));
        _cartesianPositionConstraint->update(conversion_utils_YARP::toEigen(q));
        EXPECT_GE(-_cartesianPositionConstraint->getbUpperBound()(0), previous_bUpperBound) << "@i=" << i;
        ++i;
    } while ( e > 1e-6 && i < 1000);
    ASSERT_TRUE(e < 1.51e-6);
    ASSERT_NEAR(conversion_utils_YARP::fromEigentoYarp(_DHS.leftArm->getActualPose()).getCol(3).subVector(0,2)(2),0.5,1.5e-6);
    // when at the bound, z_dot > z_dot_limit = -bUpperBound
    // and we expect z_dot_limit to be ZERO (we can go up, but not down)
    EXPECT_NEAR(-_cartesianPositionConstraint->getbUpperBound()(0),0.0,1.5e-6);


    p(2,3) = 0.45;
    _DHS.leftArm->setReference(p);

    e = 0.0;
    i = 0;
    do {
        e = sqrt(_DHS.leftArm->getb().squaredNorm());
        q += pinv(conversion_utils_YARP::fromEigentoYarp(_DHS.leftArm->getA()),1E-7)*_DHS.leftArm->getLambda()*
                conversion_utils_YARP::fromEigentoYarp(_DHS.leftArm->getb());
        coman.updateiDyn3Model(q, true);
        _DHS.leftArm->update(conversion_utils_YARP::toEigen(q));
        _cartesianPositionConstraint->update(conversion_utils_YARP::toEigen(q));
        ++i;
    } while ( e > 1e-6 && i < 1000);
    ASSERT_TRUE(e < 1.51e-6);
    // when below the bound, z_dot > z_dot_limit = -bUpperBound
    // and we expect z_dot_limit to be POSITIVE (we MUST go up, quickly!!!)
    EXPECT_GT(-_cartesianPositionConstraint->getbUpperBound()(0),0.0);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
