#include "opencv2/opencv.hpp"
#include <iostream>

#include <memory>
#include "WebcamManager.hpp"
#include "GameStrategy.hpp"
#include "RedBallGame.hpp"
#include "TransformGame.hpp"
#include "QRGame.hpp"

int main()
{
    std::cout << "Hello World! CV Version: " << CV_VERSION << std::endl;

    WebcamManager webcam;
    if (!webcam.initialize()) {
        return -1;
    }

    // 전략 패턴 개선: Context 역할을 main 루프가 수행하여 전략 간 전환 관리
    GameState currentState = GameState::RED_BALL;

    while (currentState != GameState::EXIT) {
        std::unique_ptr<GameStrategy> game;

        if (currentState == GameState::RED_BALL) {
            game = std::make_unique<RedBallGame>(webcam);
        } else if (currentState == GameState::TRANSFORM) {
            game = std::make_unique<TransformGame>(webcam);
        } else if (currentState == GameState::QR_GAME) {
            game = std::make_unique<QRGame>(webcam);
        }

        if (game) {
            currentState = game->run();
        } else {
            break;
        }
    }

    return 0;
}
