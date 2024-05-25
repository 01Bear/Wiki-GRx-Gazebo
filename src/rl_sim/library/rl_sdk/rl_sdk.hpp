#ifndef RL_SDK_HPP
#define RL_SDK_HPP

#include <torch/script.h>
#include <iostream>
#include <string>
#include <unistd.h>

#include <yaml-cpp/yaml.h>
#define CONFIG_PATH CMAKE_CURRENT_SOURCE_DIR "/config.yaml"

template<typename T>
struct RobotCommand
{
    struct MotorCommand
    {
        std::vector<T> q = std::vector<T>(32, 0.0);
        std::vector<T> dq = std::vector<T>(32, 0.0);
        std::vector<T> tau = std::vector<T>(32, 0.0);
        std::vector<T> kp = std::vector<T>(32, 0.0);
        std::vector<T> kd = std::vector<T>(32, 0.0);
    } motor_command;
};

template<typename T>
struct RobotState
{
    struct IMU
    {
        T quaternion[4] = {1.0, 0.0, 0.0, 0.0}; // w, x, y, z
        T gyroscope[3] = {0.0, 0.0, 0.0};
        T accelerometer[3] = {0.0, 0.0, 0.0};
    } imu;

    struct MotorState
    {
        std::vector<T> q = std::vector<T>(32, 0.0);
        std::vector<T> dq = std::vector<T>(32, 0.0);
        std::vector<T> ddq = std::vector<T>(32, 0.0);
        std::vector<T> tauEst = std::vector<T>(32, 0.0);
        std::vector<T> cur = std::vector<T>(32, 0.0);
    } motor_state;
};

enum STATE {
    STATE_WAITING = 0,
    STATE_POS_GETUP,
    STATE_RL_INIT,
    STATE_RL_RUNNING,
    STATE_POS_GETDOWN,
    STATE_RESET_SIMULATION,
};

struct KeyBoard
{
    STATE keyboard_state;
    double x = 0.0;
    double y = 0.0;
    double yaw = 0.0;
};

struct ModelParams
{
    std::string model_name;
    int num_observations;
    double damping;
    double stiffness;
    double action_scale;
    torch::Tensor clip_actions_max;
    torch::Tensor clip_actions_min;
    int num_of_dofs;
    double lin_vel_scale;
    double ang_vel_scale;
    double dof_pos_scale;
    double dof_vel_scale;
    double clip_obs;
    double clip_actions;
    torch::Tensor torque_limits;
    torch::Tensor rl_kd;
    torch::Tensor rl_kp;
    torch::Tensor fixed_kp;
    torch::Tensor fixed_kd;
    torch::Tensor commands_scale;
    torch::Tensor default_dof_pos;
    std::vector<std::string> joint_controller_names;
};

struct Observations
{
    torch::Tensor lin_vel;           
    torch::Tensor ang_vel;      
    torch::Tensor gravity_vec;      
    torch::Tensor commands;        
    torch::Tensor base_quat;   
    torch::Tensor dof_pos;           
    torch::Tensor dof_vel;           
    torch::Tensor actions;
};

class RL
{
public:
    RL(){};

    ModelParams params;
    Observations obs;

    RobotState<double> robot_state;
    RobotCommand<double> robot_command;

    // init
    void InitObservations();
    void InitOutputs();
    void InitKeyboard();

    // rl functions
    virtual torch::Tensor Forward() = 0;
    virtual torch::Tensor ComputeObservation() = 0;
    virtual void GetState(RobotState<double> *state) = 0;
    virtual void SetCommand(const RobotCommand<double> *command) = 0;
    void StateController(const RobotState<double> *state, RobotCommand<double> *command);
    torch::Tensor ComputeTorques(torch::Tensor actions);
    torch::Tensor ComputePosition(torch::Tensor actions);
    torch::Tensor QuatRotateInverse(torch::Tensor q, torch::Tensor v);

    // yaml params
    void ReadYaml(std::string robot_name);

    // keyboard
    KeyBoard keyboard;
    void RunKeyboard();

    // others
    std::string robot_name;
    STATE running_state = STATE_WAITING;

protected:
    // rl module
    torch::jit::script::Module model;
    // output buffer
    torch::Tensor output_torques;
    torch::Tensor output_dof_pos;
    // getup getdown buffer
    float getup_percent = 0.0;
    float getdown_percent = 0.0;
    std::vector<double> start_pos;
    std::vector<double> now_pos;
};

#endif