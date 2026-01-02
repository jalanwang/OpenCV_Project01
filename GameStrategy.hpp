#ifndef GAME_STRATEGY_HPP
#define GAME_STRATEGY_HPP

class GameStrategy {
public:
    virtual ~GameStrategy() {}
    virtual void run() = 0;
};

#endif // GAME_STRATEGY_HPP
