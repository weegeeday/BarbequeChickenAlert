#include "audio.h"
#include "game_state.h"
#include "sound_assets.h"

void initAudio() {
    soundEnable();
}

void playChickenSound() {
    if (!g_gameState.isSoundEnabled) return;
    DC_FlushRange((const void*)sound_chicken_pcm, sizeof(sound_chicken_pcm));
    soundPlaySample((const void*)sound_chicken_pcm, SoundFormat_16Bit, sizeof(sound_chicken_pcm), 11025, 127, 64, false, 0);
}

void playPopupSound() {
    if (!g_gameState.isSoundEnabled) return;
    DC_FlushRange((const void*)sound_popup_pcm, sizeof(sound_popup_pcm));
    soundPlaySample((const void*)sound_popup_pcm, SoundFormat_16Bit, sizeof(sound_popup_pcm), 11025, 127, 64, false, 0);
}

void playRarePopupSound() {
    if (!g_gameState.isSoundEnabled) return;
    DC_FlushRange((const void*)sound_rare_pcm, sizeof(sound_rare_pcm));
    soundPlaySample((const void*)sound_rare_pcm, SoundFormat_16Bit, sizeof(sound_rare_pcm), 11025, 127, 64, false, 0);
}

void playPurchaseSound() {
    if (!g_gameState.isSoundEnabled) return;
    DC_FlushRange((const void*)sound_popup_pcm, sizeof(sound_popup_pcm));
    soundPlaySample((const void*)sound_popup_pcm, SoundFormat_16Bit, sizeof(sound_popup_pcm), 11025, 127, 64, false, 0);
}
