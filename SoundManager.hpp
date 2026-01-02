#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <iostream>
#include <cstdlib>

class SoundManager {
public:
    static void playBeep() {
        // sox의 play 명령어를 사용하여 비프음 재생 (440Hz, 0.1초)
        // -q: quiet mode, -n: null input (synth), synth 0.1 sin 440: 0.1초 동안 440Hz 사인파 생성
        // 백그라운드 실행(&)을 통해 게임 딜레이 방지
        system("play -q -n synth 0.1 sin 440 &");
    }
};

#endif // SOUND_MANAGER_HPP
