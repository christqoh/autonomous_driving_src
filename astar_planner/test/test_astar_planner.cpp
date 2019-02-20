#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <costmap_2d/costmap_2d_ros.h>
#include <vector>

#include "astar_planner/astar_planner.h"
#include "astar_planner/costmap.h"
#include "astar_planner/utils.h"

using ::testing::ElementsAre;
using astar_planner::Position;

class AStarPlannerTest : public testing::Test {
protected:
    void SetUp() override {
        nh = ros::NodeHandle("~/test_planner");
        costmap = new astar_planner::EmptyCostmap(100, 100, 0.1);
    }

    void ASSERT_PLANS_EQUAL(const std::vector<Position> &actual, const std::vector<Position> &desired) const {
        ASSERT_EQ(actual.size(), desired.size());
        for (int i = 0; i < actual.size(); i++) {
            std::string msg = errorMsg(actual[i], desired[i], i);
            ASSERT_NEAR(actual[i].x, desired[i].x, costmap->getResolution()) << msg;
            ASSERT_NEAR(actual[i].y, desired[i].y, costmap->getResolution()) << msg;
            ASSERT_NEAR(actual[i].th, desired[i].th, costmap->getResolution()) << msg;
        }
    }

    std::string errorMsg(const Position &actual, const Position &desired, int i) const {
        std::ostringstream out;
        out << "Expected " << desired << " but got " << actual << " at position " << i;
        return out.str();
    }

    astar_planner::AStarPlanner planner;
    ros::NodeHandle nh;
    astar_planner::Costmap *costmap;
};


TEST_F(AStarPlannerTest, testMakePlan) {
    nh.setParam("turning_radius", 1.0);
    nh.setParam("step_size", M_PI / 2);
    planner.initialize("test_planner", costmap);
    std::vector<Position> plan;
    double e = costmap->getResolution();
    ASSERT_TRUE(planner.makePlan(Position(0.02, 0.02, 0), Position(2, M_PI + 2, 0), plan));
    ASSERT_PLANS_EQUAL(plan, {Position(0, 0, 0), Position(1, 1, M_PI / 2),
                              Position(1, 1 + M_PI / 2, M_PI / 2),
                              Position(1, 1 + M_PI, M_PI / 2),
                              Position(2, 2 + M_PI, 0)});
}

TEST_F(AStarPlannerTest, testMakePlan_StraightLine) {
    nh.setParam("turning_radius", 1.0);
    nh.setParam("step_size", 2);
    planner.initialize("test_planner", costmap);
    std::vector<Position> plan;
    ASSERT_TRUE(planner.makePlan(Position(0, 0, 0), Position(6, 0, 0), plan));
    ASSERT_THAT(plan, ElementsAre(Position(0, 0, 0), Position(2, 0, 0), Position(4, 0, 0), Position(6, 0, 0)));
}

TEST_F(AStarPlannerTest, testGetNeighbors_FaceBackwards) {
    nh.setParam("turning_radius", 2.0);
    nh.setParam("step_size", M_PI);
    planner.initialize("test_planner", costmap);
    auto neighbors = planner.getNeighbors(Position(5, 5, -M_PI));
    std::vector<Position> poses;
    auto get_pos = [](astar_planner::PosWithDist &pos) -> Position { return pos.pose; };
    std::transform(neighbors.begin(), neighbors.end(), std::back_inserter(poses), get_pos);
    ASSERT_THAT(poses, ElementsAre(Position(3, 7, -3 * M_PI / 2), Position(5 - M_PI, 5, -M_PI),
                                   Position(3, 3, -M_PI / 2)));
}

TEST_F(AStarPlannerTest, testGetNeighborsFaceForwards) {
    nh.setParam("turning_radius", 1.0);
    nh.setParam("step_size", M_PI / 2);
    planner.initialize("test_planner", costmap);
    auto neighbors = planner.getNeighbors(Position(5, 5, 0));
    std::vector<Position> poses;
    std::transform(neighbors.begin(), neighbors.end(), std::back_inserter(poses),
                   [](astar_planner::PosWithDist pos) { return pos.pose; });
    ASSERT_THAT(poses, ElementsAre(Position(6, 4, -M_PI / 2), Position(5 + M_PI / 2, 5, 0),
                                   Position(6, 6, M_PI / 2)));

}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    ros::init(argc, argv, "tester");
    ros::NodeHandle nh;
    return RUN_ALL_TESTS();
}