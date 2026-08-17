#pragma once

enum class PlaybackState
{
    Idle,

    Resolving,    // Searching YouTube / playlist

    Buffering,    // Preparing decoder

    Playing,

    Paused,

    Stopping,

    Error
};
