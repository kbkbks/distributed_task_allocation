/*
 # Copyright (c) 2019-2020 Xinyan Han. All rights reserved.
 */
#include "ccoordinatecommunication.h"

/*
 * 构造函数
 */
ccoordinatecommunication::ccoordinatecommunication(crobot * Robot) : CurrentRobot(Robot), CurrentCoorStatus(true), NewCoorTEQNumber(0) {
    switch (CurrentRobot->sendRobotNum()) {
    case 0:
        CoorStatus = {true, true};
        break;

    case 1:
    case 2:
    case 3:
    case 4:
        CoorStatus = {true, true, true};
        break;

    case 5:
        CoorStatus = {true, true};
        break;

    default:
        break;
    }
}

/*
 * 机器人协调通信
 */
void ccoordinatecommunication::enterCoordinate() {
    int CurrentTEQLength = CurrentRobot->getTaskExecutionQueueLength();
    // 计算CoorTEQ长度合法性
    for (int i = 0; i < CurrentRobot->getCoorCommunicateWidth(); ++i) {
        if (CurrentRobot->getCoorTEQLength(i) > COORDINATE_LENGTH + 1) {
            CoorTEQLengthLegality[i] = true;    // 长度合法
        } else {
            CoorTEQLengthLegality[i] = false;   // 长度非法
        }
    }

    // 判断当前任务执行队列是否大于协调长度
    if (CurrentTEQLength > COORDINATE_LENGTH + 1) {
        /*
         * 被动协调阶段
         */
        while (1) {
            // 读协调状态
            muCoorStatus.lock();
            readCoorStatus();

            // 读NewCoorTEQ(CurrentTEQ)
            readNewCoorTEQ();

            // 判断协调条件
            bool Status = checkCoorStatus();
            if (Status == true) {
                // 自身可协调，邻接机器人可协调，满足协调三规则
                if (CurrentCoorStatus) {
                    // 主动协调阶段
                    activeCoorProcess();
                } else {
                    muCoorStatus.unlock();
                    // cout << "当前机器人" << CurrentRobot->sendRobotNum() << "已完成协调" << endl;
                }
            } else {
                muCoorStatus.unlock();
                // cout << "协调条件不满足" << endl;
            }

            // 机器人协调退出条件
            if (NewCoorTEQNumber == (CoorStatus.size()-1) && CurrentCoorStatus == false) {
                break;
            }
        }
    } else {
        cout << "当前机器人任务执行队列小于协调长度，不进行协调！" << endl;
    }
}

/*
 * 状态检测
 */
bool ccoordinatecommunication::checkCoorStatus() {
    bool StatusGathered = false;
    for (auto iter = CoorStatus.begin(); iter != CoorStatus.end(); ++iter) {
        if (*iter == false) {
            StatusGathered = false;
            break;
        } else {
            StatusGathered = true;
        }
    }
    return StatusGathered;
}

/*
 * 主动协调
 */
void ccoordinatecommunication::activeCoorProcess() {
    //锁协调状态
    for (auto iter = CoorStatus.begin(); iter != CoorStatus.end(); ++iter) {
        *iter = false;
    }
    writeCoorStatus();
    muCoorStatus.unlock();

    // 检查协调对象NewCoorTEQ刷新
    checkObjectNewCoorTEQ();

    // 读CoorTEQ
    readTEQ();

    //协调算法
    cout << "机器人" << CurrentRobot->sendRobotNum() << "协调算法" << endl;
    CurrentRobot->multirobotCoordination(COORDINATE_LENGTH);

    // 写CurrentTEQ和CoorTEQ
    writeTEQ();

    // 自身协调状态置false
    CurrentCoorStatus = false;

    //恢复协调状态
    muCoorStatus.lock();
    for (auto iter = CoorStatus.begin(); iter != CoorStatus.end(); ++iter) {
        *iter = true;
    }
    writeCoorStatus();
    muCoorStatus.unlock();
}

/*
 * 读协调状态，下标0为当前机器人协调状态，下标1和2按字典序存放邻接机器人协调状态
 */
void ccoordinatecommunication::readCoorStatus() {
    switch (CurrentRobot->sendRobotNum()) {
    case 0:
        CoorStatus[0] = GlobalCoorStatus[0];
        CoorStatus[1] = GlobalCoorStatus[1];
        break;

    case 1:
        CoorStatus[0] = GlobalCoorStatus[1];
        CoorStatus[1] = GlobalCoorStatus[0];
        CoorStatus[2] = GlobalCoorStatus[2];
        break;

    case 2:
        CoorStatus[0] = GlobalCoorStatus[2];
        CoorStatus[1] = GlobalCoorStatus[1];
        CoorStatus[2] = GlobalCoorStatus[3];
        break;

    case 3:
        CoorStatus[0] = GlobalCoorStatus[3];
        CoorStatus[1] = GlobalCoorStatus[2];
        CoorStatus[2] = GlobalCoorStatus[4];
        break;

    case 4:
        CoorStatus[0] = GlobalCoorStatus[4];
        CoorStatus[1] = GlobalCoorStatus[3];
        CoorStatus[2] = GlobalCoorStatus[5];
        break;

    case 5:
        CoorStatus[0] = GlobalCoorStatus[5];
        CoorStatus[1] = GlobalCoorStatus[4];
        break;

    default:
        break;
    }
}

/*
 * 写协调状态
 */
void ccoordinatecommunication::writeCoorStatus() {
    switch (CurrentRobot->sendRobotNum()) {
    case 0:
        GlobalCoorStatus[0] = CoorStatus[0];
        GlobalCoorStatus[1] = CoorStatus[1];
        break;

    case 1:
        GlobalCoorStatus[1] = CoorStatus[0];
        GlobalCoorStatus[0] = CoorStatus[1];
        GlobalCoorStatus[2] = CoorStatus[2];
        break;

    case 2:
        GlobalCoorStatus[2] = CoorStatus[0];
        GlobalCoorStatus[1] = CoorStatus[1];
        GlobalCoorStatus[3] = CoorStatus[2];
        break;

    case 3:
        GlobalCoorStatus[3] = CoorStatus[0];
        GlobalCoorStatus[2] = CoorStatus[1];
        GlobalCoorStatus[4] = CoorStatus[2];
        break;

    case 4:
        GlobalCoorStatus[4] = CoorStatus[0];
        GlobalCoorStatus[3] = CoorStatus[1];
        GlobalCoorStatus[5] = CoorStatus[2];
        break;

    case 5:
        GlobalCoorStatus[5] = CoorStatus[0];
        GlobalCoorStatus[4] = CoorStatus[1];
        break;

    default:
        break;
    }
}

/*
 * 读CoorTEQ
 */
void ccoordinatecommunication::readTEQ() {
    switch (CurrentRobot->sendRobotNum()) {
    case 0:
    {
        // robot0向robot1进行协调，向robot[1]读数据
        unique_lock<mutex> lck1_0(TEQrw1_0);
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ1_0, 0);   //读函数
        break;
    }

    case 1:
    {
        // robot1向robot0和robot2进行协调，向robot[0]和robot[2]读数据
        unique_lock<mutex> lck0_1(TEQrw0_1);
        unique_lock<mutex> lck2_1(TEQrw2_1);
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ0_1, 0);   //读函数
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ2_1, 1);   //读函数
        break;
    }

    case 2:
    {
        // robot2向robot[1]和robot[3]进行协调，向robot[1]和robot[3]读数据
        unique_lock<mutex> lck1_2(TEQrw1_2);
        unique_lock<mutex> lck3_2(TEQrw3_2);
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ1_2, 0);   //读函数
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ3_2, 1);   //读函数
        break;
    }

    case 3:
    {
        // robot3向robot[2]和robot[4]进行协调，向robot[2]和robot[4]读数据
        unique_lock<mutex> lck2_3(TEQrw2_3);
        unique_lock<mutex> lck4_3(TEQrw4_3);
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ2_3, 0);   //读函数
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ4_3, 1);   //读函数
        break;
    }

    case 4:
    {
        // robot4向robot[3]和robot[5]进行协调，向robot[3]和robot[5]读数据
        unique_lock<mutex> lck3_4(TEQrw3_4);
        unique_lock<mutex> lck5_4(TEQrw5_4);
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ3_4, 0);   //读函数
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ5_4, 1);   //读函数
        break;
    }

    case 5:
    {
        // robot5向robot[4]进行协调，向robot[4]读数据
        unique_lock<mutex> lck4_5(TEQrw4_5);
        CurrentRobot->updateTaskExecutionQueue(GlobalTEQ4_5, 0);   //读函数
        break;
    }

    default:
        break;
    }
}

/*
 * 写CoorTEQ
 */
void ccoordinatecommunication::writeTEQ() {
    switch (CurrentRobot->sendRobotNum()) {
    case 0:
    {
        unique_lock<mutex> lck0_1(TEQrw0_1);
        unique_lock<mutex> lckNCT0_1(NewCoorTEQrw0_1);
        GlobalTEQ0_1 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalNewCoorTEQ0_1 = CurrentRobot->setNewCoorTEQ(0);  //写函数
        break;
    }

    case 1:
    {
        // robot[1]向robot[0]和robot[2]写数据，robot[0]和robot[2]进行协调
        unique_lock<mutex> lck1_0(TEQrw1_0);
        unique_lock<mutex> lck1_2(TEQrw1_2);
        unique_lock<mutex> lckNCT1_0(NewCoorTEQrw1_0);
        unique_lock<mutex> lckNCT1_2(NewCoorTEQrw1_2);
        GlobalTEQ1_0 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalTEQ1_2 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalNewCoorTEQ1_0 = CurrentRobot->setNewCoorTEQ(0);  //写函数
        GlobalNewCoorTEQ1_2 = CurrentRobot->setNewCoorTEQ(1);  //写函数
        break;
    }

    case 2:
    {
        // robot[2]向robot[1]和robot[3]写数据，robot[1]和robot[3]进行协调
        unique_lock<mutex> lck2_1(TEQrw2_1);
        unique_lock<mutex> lck2_3(TEQrw2_3);
        unique_lock<mutex> lckNCT2_1(NewCoorTEQrw2_1);
        unique_lock<mutex> lckNCT2_3(NewCoorTEQrw2_3);
        GlobalTEQ2_1 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalTEQ2_3 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalNewCoorTEQ2_1 = CurrentRobot->setNewCoorTEQ(0);  //写函数
        GlobalNewCoorTEQ2_3 = CurrentRobot->setNewCoorTEQ(1);  //写函数
        break;
    }

    case 3:
    {
        // robot3向robot[2]和robot[4]写数据，robot[2]和robot[4]进行协调
        unique_lock<mutex> lck3_2(TEQrw3_2);
        unique_lock<mutex> lck3_4(TEQrw3_4);
        unique_lock<mutex> lckNCT3_2(NewCoorTEQrw3_2);
        unique_lock<mutex> lckNCT3_4(NewCoorTEQrw3_4);
        GlobalTEQ3_2 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalTEQ3_4 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalNewCoorTEQ3_2 = CurrentRobot->setNewCoorTEQ(0);  //写函数
        GlobalNewCoorTEQ3_4 = CurrentRobot->setNewCoorTEQ(1);  //写函数
        break;
    }

    case 4:
    {
        // robot4向robot[3]和robot[5]写数据，robot[3]和robot[5]进行协调
        unique_lock<mutex> lck4_3(TEQrw4_3);
        unique_lock<mutex> lck4_5(TEQrw4_5);
        unique_lock<mutex> lckNCT4_3(NewCoorTEQrw4_3);
        unique_lock<mutex> lckNCT4_5(NewCoorTEQrw4_5);
        GlobalTEQ4_3 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalTEQ4_5 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalNewCoorTEQ4_3 = CurrentRobot->setNewCoorTEQ(0);  //写函数
        GlobalNewCoorTEQ4_5 = CurrentRobot->setNewCoorTEQ(1);  //写函数
        break;
    }

    case 5:
    {
        // robot5向robot[4]写数据，robot[4]进行协调
        unique_lock<mutex> lck5_4(TEQrw5_4);
        unique_lock<mutex> lckNCT5_4(NewCoorTEQrw5_4);
        GlobalTEQ5_4 = CurrentRobot->setTaskExecutionQueue();  //写函数
        GlobalNewCoorTEQ5_4 = CurrentRobot->setNewCoorTEQ(0);  //写函数
        break;
    }

    default:
        break;
    }
}

/*
 * readNewCoorTEQ
 */
void ccoordinatecommunication::readNewCoorTEQ() {
    switch (CurrentRobot->sendRobotNum()) {
    case 0:
    {
        if (!GlobalNewCoorTEQ1_0.empty()) {
            unique_lock<mutex> lck1_0(NewCoorTEQrw1_0);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ1_0);
            GloNewCoorTEQFlag1_0 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ1_0.clear();
            convarNCQ1_0.notify_all();
        }
        break;
    }

    case 1:
    {
        if (!GlobalNewCoorTEQ0_1.empty()) {
            unique_lock<mutex> lck0_1(NewCoorTEQrw0_1);
            unique_lock<mutex> lck1_2(TEQrw1_2);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ0_1);
            GlobalTEQ1_2 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag0_1 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ0_1.clear();
            convarNCQ0_1.notify_all();
        }
        if (!GlobalNewCoorTEQ2_1.empty()) {
            unique_lock<mutex> lck2_1(NewCoorTEQrw2_1);
            unique_lock<mutex> lckNCT1_0(NewCoorTEQrw1_0);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ2_1);
            GlobalTEQ1_0 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag2_1 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ2_1.clear();
            convarNCQ2_1.notify_all();
        }
        break;
    }

    case 2:
    {
        if (!GlobalNewCoorTEQ1_2.empty()) {
            unique_lock<mutex> lck1_2(NewCoorTEQrw1_2);
            unique_lock<mutex> lck2_3(TEQrw2_3);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ1_2);
            GlobalTEQ2_3 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag1_2 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ1_2.clear();
            convarNCQ1_2.notify_all();
        }
        if (!GlobalNewCoorTEQ3_2.empty()) {
            unique_lock<mutex> lck3_2(NewCoorTEQrw3_2);
            unique_lock<mutex> lck2_1(TEQrw2_1);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ3_2);
            GlobalTEQ2_1 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag3_2 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ3_2.clear();
            convarNCQ3_2.notify_all();
        }
        break;
    }

    case 3:
    {
        if (!GlobalNewCoorTEQ2_3.empty()) {
            unique_lock<mutex> lck2_3(NewCoorTEQrw2_3);
            unique_lock<mutex> lck3_4(TEQrw3_4);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ2_3);
            GlobalTEQ3_4 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag2_3 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ2_3.clear();
            convarNCQ2_3.notify_all();
        }

        if (!GlobalNewCoorTEQ4_3.empty()) {
            unique_lock<mutex> lck4_3(NewCoorTEQrw4_3);
            unique_lock<mutex> lck3_2(TEQrw3_2);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ4_3);
            GlobalTEQ3_2 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag4_3 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ4_3.clear();
            convarNCQ4_3.notify_all();
        }
        break;
    }

    case 4:
    {
        if (!GlobalNewCoorTEQ3_4.empty()) {
            unique_lock<mutex> lck3_4(NewCoorTEQrw3_4);
            unique_lock<mutex> lck4_5(TEQrw4_5);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ3_4);
            GlobalTEQ4_5 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag3_4 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ3_4.clear();
            convarNCQ3_4.notify_all();
        }

        if (!GlobalNewCoorTEQ5_4.empty()) {
            unique_lock<mutex> lck5_4(NewCoorTEQrw5_4);
            unique_lock<mutex> lck4_3(TEQrw4_3);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ5_4);
            GlobalTEQ4_3 = CurrentRobot->setTaskExecutionQueue();  //写函数
            GloNewCoorTEQFlag5_4 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ5_4.clear();
            convarNCQ5_4.notify_all();
        }
        break;
    }

    case 5:
    {
        if (!GlobalNewCoorTEQ4_5.empty()) {
            unique_lock<mutex> lck4_5(NewCoorTEQrw4_5);
            CurrentRobot->updateNewCoorTEQ(GlobalNewCoorTEQ4_5);
            GloNewCoorTEQFlag4_5 = true;
            ++NewCoorTEQNumber;
            GlobalNewCoorTEQ4_5.clear();
            convarNCQ4_5.notify_all();
        }

        break;
    }

    default:
        break;
    }
}

/*
 * 检查协调对象NewCoorTEQ刷新
 */
void ccoordinatecommunication::checkObjectNewCoorTEQ() {
    switch (CurrentRobot->sendRobotNum()) {
    case 0:
    {
        if (!GlobalNewCoorTEQ2_1.empty()) {
            unique_lock<mutex> lck2_1(NewCoorTEQrw2_1);
            while (!GloNewCoorTEQFlag2_1) {
                convarNCQ2_1.wait(lck2_1);
            }
            GlobalNewCoorTEQ2_1.clear();
        }
        break;
    }

    case 1:
    {
        if (!GlobalNewCoorTEQ3_2.empty()) {
            unique_lock<mutex> lck3_2(NewCoorTEQrw3_2);
            while (!GloNewCoorTEQFlag3_2) {
                convarNCQ3_2.wait(lck3_2);
            }
            GlobalNewCoorTEQ3_2.clear();
        }
        break;
    }

    case 2:
    {
        if (!GlobalNewCoorTEQ0_1.empty()) {
            unique_lock<mutex> lck0_1(NewCoorTEQrw0_1);
            while (!GloNewCoorTEQFlag0_1) {
                convarNCQ0_1.wait(lck0_1);
            }
            GlobalNewCoorTEQ0_1.clear();
        }
        if (!GlobalNewCoorTEQ4_3.empty()) {
            unique_lock<mutex> lck4_3(NewCoorTEQrw4_3);
            while (!GloNewCoorTEQFlag4_3) {
                convarNCQ4_3.wait(lck4_3);
            }
            GlobalNewCoorTEQ4_3.clear();
        }

        break;
    }

    case 3:
    {
        if (!GlobalNewCoorTEQ1_2.empty()) {
            unique_lock<mutex> lck1_2(NewCoorTEQrw1_2);
            while (!GloNewCoorTEQFlag1_2) {
                convarNCQ1_2.wait(lck1_2);
            }
            GlobalNewCoorTEQ1_2.clear();
        }

        if (!GlobalNewCoorTEQ5_4.empty()) {
            unique_lock<mutex> lck5_4(NewCoorTEQrw5_4);
            while (!GloNewCoorTEQFlag5_4) {
                convarNCQ5_4.wait(lck5_4);
            }
            GlobalNewCoorTEQ5_4.clear();
        }

        break;
    }

    case 4:
    {
        if (!GlobalNewCoorTEQ2_3.empty()) {
            unique_lock<mutex> lck2_3(NewCoorTEQrw2_3);
            while (!GloNewCoorTEQFlag2_3) {
                convarNCQ2_3.wait(lck2_3);
            }
            GlobalNewCoorTEQ2_3.clear();
        }
        break;
    }

    case 5:
    {
        if (!GlobalNewCoorTEQ3_4.empty()) {
            unique_lock<mutex> lck3_4(NewCoorTEQrw3_4);
            while (!GloNewCoorTEQFlag3_4) {
                convarNCQ3_4.wait(lck3_4);
            }
            GlobalNewCoorTEQ3_4.clear();
        }
        break;
    }

    default:
        break;
    }
}
