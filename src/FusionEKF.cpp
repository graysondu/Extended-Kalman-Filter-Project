#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  MatrixXd R_laser_ = MatrixXd(2, 2);
  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
              0, 0.0225;

  MatrixXd H_laser_ = MatrixXd(2, 4);
  H_laser_ << 1, 0, 0, 0,
              0, 1, 0, 0;

  VectorXd x_ = VectorXd(4);
  x_ << 1, 1, 1, 1;

  MatrixXd P_ = MatrixXd(4,4);
  P_ << 1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1000, 0,
        0, 0, 0, 1000;

  MatrixXd F_ = MatrixXd(4,4);
  F_ << 1, 0, 1, 0,
        0, 1, 0, 1,
        0, 0, 1, 0,
        0, 0, 0, 1;


  MatrixXd Q_ = MatrixXd(4, 4);
  Q_ <<  0, 0, 0, 0,
         0, 0, 0, 0,
         0, 0, 0, 0,
         0, 0, 0, 0;
  //initializing kalman filter object via Init method
  ekf_.Init(x_, P_, F_, H_laser_, R_laser_, Q_);
}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {


  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {

    // first measurement
    cout << "EKF: " << endl;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      ekf_.x_ << measurement_pack.raw_measurements_[0]*cos(measurement_pack.raw_measurements_[1]),
                 measurement_pack.raw_measurements_[0]*sin(measurement_pack.raw_measurements_[1]), 0, 0; // x, y, vx, vy

    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state if first measurement coming from Lidar.
      */
      ekf_.x_ << measurement_pack.raw_measurements_[0], measurement_pack.raw_measurements_[1], 0, 0;

    }

    previous_timestamp_ = measurement_pack.timestamp_;
    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  //compute the time elapsed between the current and previous measurements
  float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;	//dt - expressed in seconds

  previous_timestamp_ = measurement_pack.timestamp_;

  float dt_2 = dt * dt;
  float dt_3 = dt_2 * dt;
  float dt_4 = dt_3 * dt;

  //set new dt in F matrix
  ekf_.F_(0, 2) = dt;
  ekf_.F_(1, 3) = dt;

  //set acceleration noise components and the process covariance matrix Q
  float noise_ax = 9;
  float noise_ay = 9;

  ekf_.Q_ <<  dt_4/4*noise_ax, 0, dt_3/2*noise_ax, 0,
	      0, dt_4/4*noise_ay, 0, dt_3/2*noise_ay,
              dt_3/2*noise_ax, 0, dt_2*noise_ax, 0,
              0, dt_3/2*noise_ay, 0, dt_2*noise_ay;

  ekf_.Predict();

  /*****************************************************************************
   *  Update Step
   ****************************************************************************/

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates

      //use CalculateJacobian() method of Tools class to set Hj
      ekf_.H_ = MatrixXd(3, 4); 
      ekf_.H_ = tools.CalculateJacobian(ekf_.x_);

      //define R_laser and set variable of kalman object
      MatrixXd R_radar_ = MatrixXd(3, 3);
      //measurement covariance matrix - radar
      R_radar_ << 0.09, 0, 0,
                  0, 0.0009, 0,
                  0, 0, 0.09;
      ekf_.R_ = R_radar_;

      ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser updates

      //define H_laser and set variable of kalman object
      MatrixXd H_laser_ = MatrixXd(2, 4);
      H_laser_ << 1, 0, 0, 0,
                  0, 1, 0, 0;
      ekf_.H_ = H_laser_;

      //define R_laser and set variable of kalman object
      MatrixXd R_laser_ = MatrixXd(2, 2);
      //measurement covariance matrix - laser
      R_laser_ << 0.0225, 0,
                  0, 0.0225;
      ekf_.R_ = R_laser_;   

      ekf_.Update(measurement_pack.raw_measurements_);

  }

  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
