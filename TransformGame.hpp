#ifndef TRANSFORM_GAME_HPP
#define TRANSFORM_GAME_HPP

#include "GameStrategy.hpp"
#include "WebcamManager.hpp"
#include "SoundManager.hpp"
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
    cv::Mat currentImage; // 원본 이미지
    cv::Mat currentMask;
    cv::Point currentPos; // 원본 위치
    
    std::vector<Decoy> decoys; // 변형되어 추가된 이미지들
    
    cv::Mat prev_gray;
    int hitCount;

public:
    TransformGame(WebcamManager& wm) : webcam(wm), hitCount(0) {
        srand((unsigned int)time(0));
    }

    void addRandomDecoy(int screenWidth, int screenHeight) {
        if (currentImage.empty()) return;

        cv::Mat nextImg = currentImage.clone();
        cv::Mat nextMask = currentMask.clone();
        
        int type = rand() % 3; // 0: Shear, 1: Rotate, 2: Flip

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

    virtual void run() override {
        cv::Mat original = cv::imread("kimsh.jpg");
        if (original.empty()) {
            std::cerr << "Error: kimsh.jpg not found!" << std::endl;
            return;
        }
        
        // 원본 이미지 200x200으로 리사이즈
        cv::resize(original, original, cv::Size(200, 200));
        
        currentImage = original.clone();
        // 마스크 생성 (흰색)
        currentMask = cv::Mat(original.size(), CV_8UC1, cv::Scalar(255));

        int width = webcam.getWidth();
        int height = webcam.getHeight();
        
        // 원본 위치: 화면 중앙 고정
        currentPos = cv::Point((width - currentImage.cols) / 2, (height - currentImage.rows) / 2);

        while (true) {
            cv::Mat frame, gray_frame, diff, thresh;
            if (!webcam.getFrame(frame)) break;
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
                    
                    // 변형된 이미지 추가 (랜덤 위치)
                    addRandomDecoy(width, height);
                    
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

            // 2. 원본 이미지 그리기 (맨 위에)
            drawToFrame(frame, currentImage, currentMask, currentPos);

            cv::putText(frame, "Hit: " + std::to_string(hitCount), cv::Point(20, 30),
                cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2);

            cv::imshow("GAME", frame);
            gray_frame.copyTo(prev_gray);

            if (cv::waitKey(10) == 27) break;
        }
        cv::destroyAllWindows();
    }
};

#endif // TRANSFORM_GAME_HPP
