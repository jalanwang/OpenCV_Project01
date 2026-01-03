#ifndef QR_GAME_HPP
#define QR_GAME_HPP

#include "GameStrategy.hpp"
#include "WebcamManager.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class QRGame : public GameStrategy {
private:
    WebcamManager& webcam;
    cv::QRCodeDetector qrDecoder;

public:
    QRGame(WebcamManager& wm) : webcam(wm) {}

    virtual GameState run() override {
        std::cout << "Starting QR Game..." << std::endl;
        while (true) {
            cv::Mat frame;
            if (!webcam.getFrame(frame)) return GameState::EXIT;
            cv::flip(frame, frame, 1);

            std::vector<cv::Point> points;
            std::string data = qrDecoder.detectAndDecode(frame, points);
            
            if (data.length() > 0) {
                // QR 코드가 인식되면 박스 그리기
                if (!points.empty() && points.size() == 4) {
                    for(size_t i = 0; i < points.size(); i++) {
                        cv::line(frame, points[i], points[(i+1)%points.size()], cv::Scalar(255, 0, 0), 3);
                    }
                }
                
                // 텍스트 출력
                cv::putText(frame, "QR Code: " + data, cv::Point(50, 50),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
            } else {
                 cv::putText(frame, "Show QR Code", cv::Point(50, 50),
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            }

            cv::imshow("GAME", frame);

            int key = cv::waitKey(10);
            if (key == 27) return GameState::EXIT; // ESC to exit
            if (key == 32) return GameState::PLATE_COUNT_GAME; // Space to Plate Count Game
        }
        return GameState::EXIT;
    }
};

#endif // QR_GAME_HPP
