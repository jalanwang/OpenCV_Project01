#ifndef RED_BALL_GAME_HPP
#define RED_BALL_GAME_HPP

#include "GameStrategy.hpp"
#include "WebcamManager.hpp"
#include "SoundManager.hpp"
#include "KimFace.hpp"
#include "opencv2/opencv.hpp"
#include <vector>
#include <iostream>
#include <ctime>

struct Ball {
    cv::Point position; // x,y
    int radius; // 반지름
    bool active; // 움직이고 있는지
    Ball() {
        this->position = cv::Point();
        this->radius = 0;
        this->active = false;
    }
};

class RedBallGame : public GameStrategy {
private:
    WebcamManager& webcam;
    Ball redBall;
    KimFace kimFace;
    bool useFaceMode;
    double rotationAngle;
    cv::Mat prev_gray;
    int score;

    cv::Point getRandomPosition(int width, int height, int radius) {
        int x = rand() % (width - 2 * radius) + radius;
        int y = rand() % (height - 2 * radius) + radius;
        return cv::Point(x, y);
    }

public:
    RedBallGame(WebcamManager& wm) : webcam(wm), score(0), useFaceMode(false), rotationAngle(0.0) {
        srand((unsigned int)time(0));
    }

    virtual GameState run() override {
        kimFace.load("kimsh.jpg");
        int width = webcam.getWidth();
        int height = webcam.getHeight();

        redBall.radius = 20;
        redBall.position = getRandomPosition(width, height, redBall.radius);

        while (true) {
            cv::Mat frame, gray_frame, diff, thresh;
            if (!webcam.getFrame(frame)) return GameState::EXIT;

            cv::flip(frame, frame, 1); // 반전

            // 움직임 감지
            cv::cvtColor(frame, gray_frame, cv::COLOR_BGR2GRAY);
            cv::GaussianBlur(gray_frame, gray_frame, cv::Size(15, 15), 0);

            if (prev_gray.empty()) {
                gray_frame.copyTo(prev_gray);
                continue;
            }

            cv::absdiff(prev_gray, gray_frame, diff);
            cv::threshold(diff, thresh, 25.0, 255.0, cv::THRESH_BINARY);

            if (!redBall.active) {
                int x1 = cv::max(0, redBall.position.x - redBall.radius);
                int y1 = cv::max(0, redBall.position.y - redBall.radius);
                int x2 = cv::min(width, redBall.position.x + redBall.radius);
                int y2 = cv::min(height, redBall.position.y + redBall.radius);
                cv::Rect ballRect(x1, y1, x2 - x1, y2 - y1);

                cv::Mat roi = thresh(ballRect);
                int movementPixels = cv::countNonZero(roi);

                int area = (redBall.radius * 2) * (redBall.radius * 2);
                if (movementPixels > area * 0.1) {
                    SoundManager::playBeep();
                    // Face Mode에서는 색상 변경(hit) 제거
                    std::cout << "터치 " << score++ << "\r\n";
                    redBall.position = getRandomPosition(width, height, redBall.radius);
                }
            }

            if (useFaceMode) {
                rotationAngle += 10.0; // 회전
                kimFace.draw(frame, redBall.position, redBall.radius, rotationAngle);
            } else {
                cv::circle(frame, redBall.position, redBall.radius, cv::Scalar(0, 0, 255), -1);
            }
            
            cv::putText(frame, "Score : " + std::to_string(score), cv::Point(20, 30),
                cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2);
            
            if (useFaceMode) {
                cv::putText(frame, "Mode: Face", cv::Point(20, 60), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 255, 255), 2);
            } else {
                cv::putText(frame, "Mode: Ball", cv::Point(20, 60), cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0, 255, 255), 2);
            }

            cv::namedWindow("GAME");
            cv::imshow("GAME", frame);
            gray_frame.copyTo(prev_gray);

            int key = cv::waitKey(10);
            if (key == 27) // ESC
                return GameState::EXIT;
            else if (key == 32) { // Space
                if (!useFaceMode) {
                    useFaceMode = true;
                } else {
                    cv::destroyAllWindows();
                    return GameState::TRANSFORM;
                }
            }
        }
        cv::destroyAllWindows();
        return GameState::EXIT;
    }
};

#endif // RED_BALL_GAME_HPP
