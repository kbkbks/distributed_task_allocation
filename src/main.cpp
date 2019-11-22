/*
 * Xinyan Han
 * 2019-10-29
 * metatron96@126.com
 * info:
 * 分布式任务处理与竞拍机制（version 3.0）,linux版
 * 该程序测试在多个任务点同时发布一些任务后，机器人按照竞拍算法分配任务（机器人为分布式，任务发布点为集中式）。
 * 目前程序包含任务之间的协调作用，可一定程度上提高程序的分配质量。
 * 修改通信部分，两个机器人之间使用两个全局互斥量传递价格，采用双向通信。
 * 加入Bidder一致性原则，用以保证分配的一致性。
 * 针对eps加入随机量，使竞拍机器人之间不太会出现等价情形。
 * 修改之前存在的竞拍算法收敛异常问题。
 * 将原先Windows系统上的本程序，重写至linux系统。
 * -------------------------------------------------------------------------------------------------------------------------
 * 目前的程序在一个进程中执行，利用多线程技术模拟多机器人，多机器人之间通信采用线程通信（全局变量），线程并发异步IO。
 * 程序当中设计的任务发布点和任务列表为集中式，机器人为分布式。
 * 当前版本程序采用D.P.Bertskas的竞拍算法，可以实现单局竞拍最优。
 */

#include "define.h"
#include "ctaskpoint.h"
#include "crobot.h"
#include "ctasklist.h"

mutex Mymutex;  //互斥量，生成价值列表
mutex Mymutex0_1;   //互斥量，Robot[0]向Robot[1]写，Robot[1]向Robot[0]读
mutex Mymutex1_2;   //互斥量，Robot[1]向Robot[2]写，Robot[2]向Robot[1]读
mutex Mymutex2_3;   //互斥量，Robot[2]向Robot[3]写，Robot[3]向Robot[2]读
mutex Mymutex3_4;   //互斥量，Robot[3]向Robot[4]写，Robot[4]向Robot[3]读
mutex Mymutex4_5;   //互斥量，Robot[4]向Robot[5]写，Robot[5]向Robot[4]读
mutex Mymutex1_0;   //互斥量，Robot[1]向Robot[0]写，Robot[0]向Robot[1]读
mutex Mymutex2_1;   //互斥量，Robot[2]向Robot[1]写，Robot[1]向Robot[2]读
mutex Mymutex3_2;   //互斥量，Robot[3]向Robot[2]写，Robot[2]向Robot[3]读
mutex Mymutex4_3;   //互斥量，Robot[4]向Robot[3]写，Robot[3]向Robot[4]读
mutex Mymutex5_4;   //互斥量，Robot[5]向Robot[4]写，Robot[4]向Robot[5]读

vector<float> GlobalPrice0_1;   //全局价格，Robot[0]向Robot[1]写，Robot[1]向Robot[0]读
vector<float> GlobalPrice1_2;   //全局价格，Robot[1]向Robot[2]写，Robot[2]向Robot[1]读
vector<float> GlobalPrice2_3;   //全局价格，Robot[2]向Robot[3]写，Robot[3]向Robot[2]读
vector<float> GlobalPrice3_4;   //全局价格，Robot[3]向Robot[4]写，Robot[4]向Robot[3]读
vector<float> GlobalPrice4_5;   //全局价格，Robot[4]向Robot[5]写，Robot[5]向Robot[4]读

vector<int> GlobalBidder0_1;    //全局竞标者，Robot[0]向Robot[1]写，Robot[1]向Robot[0]读
vector<int> GlobalBidder1_2;    //全局竞标者，Robot[1]向Robot[2]写，Robot[2]向Robot[1]读
vector<int> GlobalBidder2_3;    //全局竞标者，Robot[2]向Robot[3]写，Robot[3]向Robot[2]读
vector<int> GlobalBidder3_4;    //全局竞标者，Robot[3]向Robot[4]写，Robot[4]向Robot[3]读
vector<int> GlobalBidder4_5;    //全局竞标者，Robot[4]向Robot[5]写，Robot[5]向Robot[4]读

vector<vector<float>> GlobalAllRobotPrice0_1(ROBOTNUM);   //全局所有机器人价格，Robot[0]向Robot[1]写，Robot[1]向Robot[0]读
vector<vector<float>> GlobalAllRobotPrice1_2(ROBOTNUM);   //全局所有机器人价格，Robot[1]向Robot[2]写，Robot[2]向Robot[1]读
vector<vector<float>> GlobalAllRobotPrice2_3(ROBOTNUM);   //全局所有机器人价格，Robot[2]向Robot[3]写，Robot[3]向Robot[2]读
vector<vector<float>> GlobalAllRobotPrice3_4(ROBOTNUM);   //全局所有机器人价格，Robot[3]向Robot[4]写，Robot[4]向Robot[3]读
vector<vector<float>> GlobalAllRobotPrice4_5(ROBOTNUM);   //全局所有机器人价格，Robot[4]向Robot[5]写，Robot[5]向Robot[4]读

vector<vector<int>> GlobalAllRobotBidder0_1(ROBOTNUM);    //全局所有机器人竞标者，Robot[0]向Robot[1]写，Robot[1]向Robot[0]读
vector<vector<int>> GlobalAllRobotBidder1_2(ROBOTNUM);    //全局所有机器人竞标者，Robot[1]向Robot[2]写，Robot[2]向Robot[1]读
vector<vector<int>> GlobalAllRobotBidder2_3(ROBOTNUM);    //全局所有机器人竞标者，Robot[2]向Robot[3]写，Robot[3]向Robot[2]读
vector<vector<int>> GlobalAllRobotBidder3_4(ROBOTNUM);    //全局所有机器人竞标者，Robot[3]向Robot[4]写，Robot[4]向Robot[3]读
vector<vector<int>> GlobalAllRobotBidder4_5(ROBOTNUM);    //全局所有机器人竞标者，Robot[4]向Robot[5]写，Robot[5]向Robot[4]读

vector<float> GlobalPrice1_0;   //全局价格，Robot[1]向Robot[0]写，Robot[0]向Robot[1]读
vector<float> GlobalPrice2_1;   //全局价格，Robot[2]向Robot[1]写，Robot[1]向Robot[2]读
vector<float> GlobalPrice3_2;   //全局价格，Robot[3]向Robot[2]写，Robot[2]向Robot[3]读
vector<float> GlobalPrice4_3;   //全局价格，Robot[4]向Robot[3]写，Robot[3]向Robot[4]读
vector<float> GlobalPrice5_4;   //全局价格，Robot[5]向Robot[4]写，Robot[4]向Robot[5]读

vector<int> GlobalBidder1_0;    //全局竞标者，Robot[1]向Robot[0]写，Robot[0]向Robot[1]读
vector<int> GlobalBidder2_1;    //全局竞标者，Robot[2]向Robot[1]写，Robot[1]向Robot[2]读
vector<int> GlobalBidder3_2;    //全局竞标者，Robot[3]向Robot[2]写，Robot[2]向Robot[3]读
vector<int> GlobalBidder4_3;    //全局竞标者，Robot[4]向Robot[3]写，Robot[3]向Robot[4]读
vector<int> GlobalBidder5_4;    //全局竞标者，Robot[5]向Robot[4]写，Robot[4]向Robot[5]读

vector<vector<float>> GlobalAllRobotPrice1_0(ROBOTNUM);   //全局所有机器人价格，Robot[1]向Robot[0]写，Robot[0]向Robot[1]读
vector<vector<float>> GlobalAllRobotPrice2_1(ROBOTNUM);   //全局所有机器人价格，Robot[2]向Robot[1]写，Robot[1]向Robot[2]读
vector<vector<float>> GlobalAllRobotPrice3_2(ROBOTNUM);   //全局所有机器人价格，Robot[3]向Robot[2]写，Robot[2]向Robot[3]读
vector<vector<float>> GlobalAllRobotPrice4_3(ROBOTNUM);   //全局所有机器人价格，Robot[4]向Robot[3]写，Robot[3]向Robot[4]读
vector<vector<float>> GlobalAllRobotPrice5_4(ROBOTNUM);   //全局所有机器人价格，Robot[5]向Robot[4]写，Robot[4]向Robot[5]读

vector<vector<int>> GlobalAllRobotBidder1_0(ROBOTNUM);    //全局所有机器人竞标者，Robot[1]向Robot[0]写，Robot[0]向Robot[1]读
vector<vector<int>> GlobalAllRobotBidder2_1(ROBOTNUM);    //全局所有机器人竞标者，Robot[2]向Robot[1]写，Robot[1]向Robot[2]读
vector<vector<int>> GlobalAllRobotBidder3_2(ROBOTNUM);    //全局所有机器人竞标者，Robot[3]向Robot[2]写，Robot[2]向Robot[3]读
vector<vector<int>> GlobalAllRobotBidder4_3(ROBOTNUM);    //全局所有机器人竞标者，Robot[4]向Robot[3]写，Robot[3]向Robot[4]读
vector<vector<int>> GlobalAllRobotBidder5_4(ROBOTNUM);    //全局所有机器人竞标者，Robot[5]向Robot[4]写，Robot[4]向Robot[5]读

//获得时间当前时间戳（毫秒级）
time_t getTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());//获取当前时间点
    std::time_t timestamp =  tp.time_since_epoch().count(); //计算距离1970-1-1,00:00的时间长度
    return timestamp;
}

void setGlobalInitialValue(int tasklist_num)
{
    //全局变量GlobalAllRobotPrice赋初值
    for (int i = 0; i < ROBOTNUM; i++)
    {
        for (int j = 0; j < tasklist_num; j++)
        {
            GlobalAllRobotPrice0_1[i].push_back(0);
            GlobalAllRobotPrice1_2[i].push_back(0);
            GlobalAllRobotPrice2_3[i].push_back(0);
            GlobalAllRobotPrice3_4[i].push_back(0);
            GlobalAllRobotPrice4_5[i].push_back(0);
            GlobalAllRobotPrice1_0[i].push_back(0);
            GlobalAllRobotPrice2_1[i].push_back(0);
            GlobalAllRobotPrice3_2[i].push_back(0);
            GlobalAllRobotPrice4_3[i].push_back(0);
            GlobalAllRobotPrice5_4[i].push_back(0);
        }
    }

    //全局变量GlobalAllRobotBidder赋初值
    for (int i = 0; i < ROBOTNUM; i++)
    {
        for (int j = 0; j < tasklist_num; j++)
        {
            GlobalAllRobotBidder0_1[i].push_back(-1);
            GlobalAllRobotBidder1_2[i].push_back(-1);
            GlobalAllRobotBidder2_3[i].push_back(-1);
            GlobalAllRobotBidder3_4[i].push_back(-1);
            GlobalAllRobotBidder4_5[i].push_back(-1);
            GlobalAllRobotBidder1_0[i].push_back(-1);
            GlobalAllRobotBidder2_1[i].push_back(-1);
            GlobalAllRobotBidder3_2[i].push_back(-1);
            GlobalAllRobotBidder4_3[i].push_back(-1);
            GlobalAllRobotBidder5_4[i].push_back(-1);
        }
    }
}

void clearGlobalVar()
{
    //清空全局价格，全局竞标者，全局所有机器人价格
    GlobalPrice0_1.clear();
    GlobalPrice1_2.clear();
    GlobalPrice2_3.clear();
    GlobalPrice3_4.clear();
    GlobalPrice4_5.clear();
    GlobalBidder0_1.clear();
    GlobalBidder1_2.clear();
    GlobalBidder2_3.clear();
    GlobalBidder3_4.clear();
    GlobalBidder4_5.clear();
    GlobalPrice1_0.clear();
    GlobalPrice2_1.clear();
    GlobalPrice3_2.clear();
    GlobalPrice4_3.clear();
    GlobalPrice5_4.clear();
    GlobalBidder1_0.clear();
    GlobalBidder2_1.clear();
    GlobalBidder3_2.clear();
    GlobalBidder4_3.clear();
    GlobalBidder5_4.clear();

    for (int i = 0; i < ROBOTNUM; i++)
    {
        GlobalAllRobotPrice0_1[i].clear();
        GlobalAllRobotPrice1_2[i].clear();
        GlobalAllRobotPrice2_3[i].clear();
        GlobalAllRobotPrice3_4[i].clear();
        GlobalAllRobotPrice4_5[i].clear();
        GlobalAllRobotPrice1_0[i].clear();
        GlobalAllRobotPrice2_1[i].clear();
        GlobalAllRobotPrice3_2[i].clear();
        GlobalAllRobotPrice4_3[i].clear();
        GlobalAllRobotPrice5_4[i].clear();
        GlobalAllRobotBidder0_1[i].clear();
        GlobalAllRobotBidder1_2[i].clear();
        GlobalAllRobotBidder2_3[i].clear();
        GlobalAllRobotBidder3_4[i].clear();
        GlobalAllRobotBidder4_5[i].clear();
        GlobalAllRobotBidder1_0[i].clear();
        GlobalAllRobotBidder2_1[i].clear();
        GlobalAllRobotBidder3_2[i].clear();
        GlobalAllRobotBidder4_3[i].clear();
        GlobalAllRobotBidder5_4[i].clear();
    }
}

int main()
{   
    //时间戳
    std::time_t timestampstart;
    std::time_t timestampstop;

    cout << "****************************" << endl;
	cout << "分布式任务处理与竞拍机制算法" << endl;
	cout << "****************************" << endl;
	cout << endl;

    //任务点对象初始化参数（任务起/终点位置）
    //该任务点的任务发布方向为，二维平面Y轴反方向
    float BeginInitial[TASKPOINT][TASKCAPACITY][2];
    float EndInitial[TASKPOINT][TASKCAPACITY][2];
    for (int j = 0; j < TASKPOINT; j++)
    {
        for (int i = 0; i < TASKCAPACITY; i++)
        {
            BeginInitial[j][i][0] = j;  //任务起点x坐标
            BeginInitial[j][i][1] = 3;  //任务起点y坐标
            EndInitial[j][i][0] = j;    //任务终点x坐标
            EndInitial[j][i][1] = 4 + i;    //任务终点y坐标
        }
    }

    //定义任务发布点，一共8个
    ctaskpoint * TaskPoint = new ctaskpoint[TASKPOINT];
    assert(TaskPoint != NULL);
    //向任务点存放初始化任务
    for (int i = 0; i < TASKPOINT; i++)
    {
        TaskPoint[i].setInitialValue(i, BeginInitial[i], EndInitial[i]);
        TaskPoint[i].printTaskRepository();
    }

    //定义机器人，一共6个
    crobot * Robot = new crobot[ROBOTNUM];
    assert(Robot != NULL);
    //设置机器人对象的初始化参数（编号，初始位置）
    for (int i = 0; i < ROBOTNUM; i++)
    {
        Robot[i].setInitialValue(i, i, 1);
        Robot[i].printRobotInfo();
    }

    //定义任务列表
    int TaskListNum(0); //任务列表中的任务数量
    ctasklist * TaskList = new ctasklist;
    assert(TaskList != NULL);
    //任务列表接收任务
    for (int i = 0; i < TASKPOINT; i++)
    {
        TaskList->getTask(TaskPoint[i], 0);
    }

    timestampstart = getTimeStamp();
    //k为任务点发布的任务编号，任务分配算法一共进行k次分配
    for (int k = 1; k < TASKCAPACITY + 1; k++)
    {
        TaskListNum = TaskList->sendTaskNumber();
        cout << "主线程ID：" << this_thread::get_id() << endl;

        //全局变量赋初值
        setGlobalInitialValue(TaskListNum);

        //创建子线程Future对象
        vector<future<crobot>> FuturesRobot;

        promise<crobot> PromisesRobot[ROBOTNUM];
        for (int i = 0; i < ROBOTNUM; i++)
        {
            FuturesRobot.push_back(PromisesRobot[i].get_future());
        }

        //创建机器人子线程
        vector<thread> ThreadsRobot;
        for (int i = 0; i < ROBOTNUM; i++)
        {
            float random_num = RAND_NUM;    //随机数，0~0.1
            //cout << random_num << endl;
            //子线程入口函数第三个参数，rand_num,可以不使用（在子线程内部定义，更符合分布式系统）
            ThreadsRobot.push_back(thread(&crobot::generateValueList, Robot[i], TaskList, TaskListNum, random_num, ref(PromisesRobot[i])));
            assert(ThreadsRobot[i].joinable()); //断言，确认机器子线程已经创建
        }

        //子线程返回被调用对象
        for (int i = 0; i < ROBOTNUM; i++)
        {
            Robot[i] = FuturesRobot[i].get();
        }

        //回收未分配的任务
        if ((GlobalBidder0_1 == GlobalBidder1_2) &&
            (GlobalBidder1_2 == GlobalBidder2_3) &&
            (GlobalBidder2_3 == GlobalBidder3_4) &&
            (GlobalBidder3_4 == GlobalBidder4_5))
            //所有机器人分配情况相同
        {
            //打印分配情况
            cout << "**********************************************************************" << endl;
            cout << "所有机器人分配情况" << endl;
            float ValueSum = 0; //价值总和
            for (int i = 0; i < ROBOTNUM; i++)
            {
                Robot[i].printAssignedTask(TaskList);
                ValueSum += Robot[i].sendAssignedTaskValue();
            }
            cout << "任务价值总和：" << ValueSum << endl;
            
            //打印所有机器人任务执行队列的总价值
            cout << "所有机器人任务执行队列价值：" << endl;
            float TotalTaskExecutionQueueValueSum = 0;  //所有机器人价值总和
            for (int i = 0; i < ROBOTNUM; i++)
            {
                float TotalTaskExecutionQueueValue = 0;
                TotalTaskExecutionQueueValue = Robot[i].sendTaskExecutionQueueValue();
                TotalTaskExecutionQueueValueSum += TotalTaskExecutionQueueValue;
                cout << "机器人" << Robot[i].sendRobotNum() << "任务执行队列总价值" << TotalTaskExecutionQueueValue << endl;
            }
            cout << "所有机器人任务执行队列价值总和为：" << " " << TotalTaskExecutionQueueValueSum << endl; 
            cout << "**********************************************************************" << endl;
        }
        else
        {
            cout << "***********************机器人分配情况不一致！*************************" << endl;
        }

        //返回剩余任务
        ctasklist * TaskList2 = new ctasklist;  //定义新的任务列表，接收剩余任务
        int TaskListResidualNum;    //剩余任务数量
        TaskListResidualNum = Robot[0].sendResidualNum();   //返回剩余任务数量
        for (int i = 0; i < TaskListResidualNum; i++)
        {
            TaskList2->getTask(Robot[0].sendResidualTask(TaskList, i));
        }
        delete TaskList;
        TaskList = TaskList2;
        
        //清空全局变量
        clearGlobalVar();
        
        //重置机器人
        for (int i = 0; i < ROBOTNUM; i++)
        {
            Robot[i].clearPropertity();
        }

        //发布新任务
        for (int i = 0; i < TASKPOINT; i++)
        {
            TaskList->getTask(TaskPoint[i], k);
        }
        
        //join主线程
        for (auto iter = ThreadsRobot.begin(); iter != ThreadsRobot.end(); iter++)
        {
            iter->join();
        }
    }
    timestampstop = getTimeStamp();

    cout << "****************************" << endl;
	cout << "*******机器人分配完毕*******" << endl;
    cout << "程序运行所需时间：" << timestampstop - timestampstart << endl;
	cout << "****************************" << endl;
    cout << endl;

    delete TaskList;
    delete[] Robot;
    delete[] TaskPoint;

    getchar();
    return 0;
}