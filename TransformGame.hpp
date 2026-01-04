#ifndef TRANSFORM_GAME_HPP
#define TRANSFORM_GAME_HPP

#include "GameStrategy.hpp"
#include "WebcamManager.hpp"
#include "SoundManager.hpp"
#include "KimFace.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <ctime>

struct Decoy {
    cv::Mat img;
    cv::Mat mask;
    cv::Point pos;
};

class TransformGame : public GameStrategy {
private:
    WebcamManager& webcam;
    KimFace kimFace; // KimFace 사용
    cv::Mat currentMask;
    cv::Point currentPos; // 원본 위치
    
    std::vector<Decoy> decoys; // 변형되어 추가된 이미지들
    
    cv::Mat prev_gray;
    int hitCount;

public:
    TransformGame(WebcamManager& wm) : webcam(wm), hitCount(0) {
        srand((unsigned int)time(0));
    }

    void addRandomDecoy(int screenWidth, int screenHeight, cv::Mat sourceImg) {
        if (sourceImg.empty()) return;

        cv::Mat nextImg = sourceImg.clone();
        cv::Mat nextMask = currentMask.clone();

        // 랜덤 크기 조절 (0.5 ~ 1.5배)
        double scale = (rand() % 101 + 50) / 100.0; // 0.5 ~ 1.5
        cv::resize(nextImg, nextImg, cv::Size(), scale, scale);
        cv::resize(nextMask, nextMask, cv::Size(), scale, scale);
        
        int type = rand() % 5; // 0: Shear, 1: Rotate, 2: Flip, 3: Grayscale, 4: Invert

        if (type == 0) { // Shear (전단)
            double shx = (rand() % 100 - 50) / 200.0; // -0.25 ~ 0.25
            double shy = (rand() % 100 - 50) / 200.0;
            
            cv::Mat M = (cv::Mat_<double>(2, 3) << 1, shx, 0, shy, 1, 0);
            cv::warpAffine(nextImg, nextImg, M, nextImg.size());
            cv::warpAffine(nextMask, nextMask, M, nextMask.size());
        }
        else if (type == 1) { // Rotate (회전)
            double angle = (rand() % 360); // 0 ~ 360도
            cv::Point2f center(nextImg.cols / 2.0f, nextImg.rows / 2.0f);
            cv::Mat M = cv::getRotationMatrix2D(center, angle, 1.0);
            
            cv::warpAffine(nextImg, nextImg, M, nextImg.size());
            cv::warpAffine(nextMask, nextMask, M, nextMask.size());
        }
        else if (type == 2) { // Flip (반전)
            int flipCode = (rand() % 3) - 1; // -1, 0, 1
            cv::flip(nextImg, nextImg, flipCode);
            cv::flip(nextMask, nextMask, flipCode);
        }
        else if (type == 3) { // Grayscale (흑백)
            cv::Mat gray;
            cv::cvtColor(nextImg, gray, cv::COLOR_BGR2GRAY);
            cv::cvtColor(gray, nextImg, cv::COLOR_GRAY2BGR); // 다시 3채널로 변환해야 copyTo 가능
        }
        else if (type == 4) { // Color Inversion (색 반전)
            cv::bitwise_not(nextImg, nextImg);
        }

        // 랜덤 위치 (화면 전체 영역 중 랜덤)
        int maxX = std::max(0, screenWidth - nextImg.cols);
        int maxY = std::max(0, screenHeight - nextImg.rows);
        
        int x = rand() % (maxX + 1);
        int y = rand() % (maxY + 1);
        
        decoys.push_back({nextImg, nextMask, cv::Point(x, y)});
    }

    // 이미지를 프레임에 합성하는 헬퍼 함수
    void drawToFrame(cv::Mat& frame, const cv::Mat& img, const cv::Mat& mask, cv::Point pos) {
        cv::Rect imgRect(pos, img.size());
        cv::Rect frameRect(0, 0, frame.cols, frame.rows);
        cv::Rect intersection = imgRect & frameRect;
        
        if (intersection.area() > 0) {
            cv::Rect srcRect(intersection.x - pos.x, intersection.y - pos.y, intersection.width, intersection.height);
            
            // 안전 장치
            if (srcRect.width <= 0 || srcRect.height <= 0) return;

            cv::Mat srcRoi = img(srcRect);
            cv::Mat maskRoi = mask(srcRect);
            cv::Mat dstRoi = frame(intersection);
            
            srcRoi.copyTo(dstRoi, maskRoi);
        }
    }

    virtual GameState run() override {
        if (!kimFace.load("kimsh.jpg")) {
            std::cerr << "Error: kimsh.jpg not found!" << std::endl;
            return GameState::EXIT;
        }
        
        // 원본 이미지 200x200으로 리사이즈 (KimFace 내부 이미지 변경)
        kimFace.resize(200, 200);
        
        // 마스크 생성 (흰색) - 초기 이미지 기준
        cv::Mat tempImg = kimFace.getCurrentImage();
        currentMask = cv::Mat(tempImg.size(), CV_8UC1, cv::Scalar(255));

        int width = webcam.getWidth();
        int height = webcam.getHeight();
        
        // 원본 위치: 화면 중앙 고정
        currentPos = cv::Point((width - tempImg.cols) / 2, (height - tempImg.rows) / 2);

        cv::namedWindow("GAME");
        cv::moveWindow("GAME", 0, 0);

        while (true) {
            cv::Mat frame, gray_frame, diff, thresh;
            if (!webcam.getFrame(frame)) return GameState::EXIT;
            cv::flip(frame, frame, 1);

            cv::cvtColor(frame, gray_frame, cv::COLOR_BGR2GRAY);
            cv::GaussianBlur(gray_frame, gray_frame, cv::Size(15, 15), 0);

            if (prev_gray.empty()) {
                gray_frame.copyTo(prev_gray);
                continue;
            }

            cv::absdiff(prev_gray, gray_frame, diff);
            cv::threshold(diff, thresh, 25.0, 255.0, cv::THRESH_BINARY);

            // 충돌 체크 (원본 이미지만 판정)
            cv::Mat currentImage = kimFace.getCurrentImage(); // 현재 색상의 이미지 가져오기
            cv::Rect imgRect(currentPos, currentImage.size());
            cv::Rect frameRect(0, 0, width, height);
            cv::Rect intersection = imgRect & frameRect;

            if (intersection.area() > 0) {
                cv::Mat roi = thresh(intersection);
                int movementPixels = cv::countNonZero(roi);
                
                // 민감도: 영역 크기의 10% 이상 움직임
                if (movementPixels > intersection.area() * 0.1) {
                    SoundManager::playBeep();
                    hitCount++;
                    std::cout << "Hit! " << hitCount << std::endl;
                    
                    // 1. 색상 변경 (Morphing)
                    kimFace.hit();
                    
                    // 2. 변형된 이미지 추가 (Popping/Transform) - 변경된 색상으로
                    addRandomDecoy(width, height, kimFace.getCurrentImage());
                    
                    // 연속 히트 방지를 위한 짧은 대기 및 배경 갱신
                    cv::waitKey(100);
                    gray_frame.copyTo(prev_gray); 
                }
            }

            // 그리기
            // 1. 변형된 이미지들 (Decoys) 먼저 그리기 (배경처럼)
            for (const auto& decoy : decoys) {
                drawToFrame(frame, decoy.img, decoy.mask, decoy.pos);
            }

            // 2. 원본 이미지 그리기 (맨 위에) - 현재 색상 반영
            drawToFrame(frame, kimFace.getCurrentImage(), currentMask, currentPos);

            cv::putText(frame, "Hit: " + std::to_string(hitCount), cv::Point(20, 30),
                cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2);

            cv::imshow("GAME", frame);
            gray_frame.copyTo(prev_gray);

            int key = cv::waitKey(10);
            if (key == 27) return GameState::EXIT;
            if (key == 32) return GameState::QR_GAME;
        }
        cv::destroyAllWindows();
        return GameState::EXIT;
    }
};

#endif // TRANSFORM_GAME_HPP
