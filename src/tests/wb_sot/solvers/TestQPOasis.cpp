#include <gtest/gtest.h>
#include <wb_sot/solvers/QPOases.h>
#include <yarp/sig/all.h>

namespace {

class simpleProblem
{
public:
    simpleProblem():
        H(2,2),
        g(2),
        A(2,2),
        l(2), u(2), lA(2), uA(2),
        ht(qpOASES::HST_IDENTITY)
    {
        H = H.eye();
        g[0] = -5.0; g[1] = 5.0;
        A.zero();
        l[0] = -10.0; l[1] = -10.0;
        u[0] = 10.0; u[1] = 10.0;
        lA[0] = -10.0; lA[1] = -10.0;
        uA[0] = 10.0; uA[1] = 10.0;

    }

    yarp::sig::Matrix H;
    yarp::sig::Vector g;
    yarp::sig::Matrix A;
    yarp::sig::Vector l;
    yarp::sig::Vector u;
    yarp::sig::Vector lA;
    yarp::sig::Vector uA;
    qpOASES::HessianType ht;
};

class testQPOasesProblem: public ::testing::Test,
        public wb_sot::solvers::QPOasesProblem
{
protected:

    testQPOasesProblem()
    {

    }

    void setTestProblem(const qpOASES::SQProblem &problem)
    {
        this->addProblem(problem);
    }

    virtual ~testQPOasesProblem() {

    }

    virtual void SetUp() {

    }

    virtual void TearDown() {

    }

};


TEST_F(testQPOasesProblem, testSimpleProblem)
{
    yarp::sig::Vector x(2);
    simpleProblem sp;

    qpOASES::SQProblem testProblem(x.size(), sp.A.rows(), sp.ht);
    this->setTestProblem(testProblem);

    this->initProblem(sp.H, sp.g, sp.A, sp.lA, sp.uA, sp.l, sp.u);

    EXPECT_TRUE(this->solve());
    EXPECT_TRUE(this->isQProblemInitialized());
    yarp::sig::Vector s = this->getSolution();
    EXPECT_EQ(-sp.g[0], s[0]);
    EXPECT_EQ(-sp.g[1], s[1]);

    for(unsigned int i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(this->solve());

        yarp::sig::Vector s = this->getSolution();
        EXPECT_EQ(-sp.g[0], s[0]);
        EXPECT_EQ(-sp.g[1], s[1]);
    }

}

TEST_F(testQPOasesProblem, testUpdatedProblem)
{
    yarp::sig::Vector x(2);
    simpleProblem sp;

    qpOASES::SQProblem testProblem(x.size(), sp.A.rows(), sp.ht);
    this->setTestProblem(testProblem);

    this->initProblem(sp.H, sp.g, sp.A, sp.lA, sp.uA, sp.l, sp.u);

    EXPECT_TRUE(this->solve());
    yarp::sig::Vector s = this->getSolution();
    EXPECT_EQ(-sp.g[0], s[0]);
    EXPECT_EQ(-sp.g[1], s[1]);

    sp.g[0] = -1.0; sp.g[1] = 1.0;
    EXPECT_TRUE(this->updateTask(sp.H, sp.g));
    EXPECT_TRUE(this->solve());

    s = this->getSolution();
    EXPECT_EQ(-sp.g[0], s[0]);
    EXPECT_EQ(-sp.g[1], s[1]);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}