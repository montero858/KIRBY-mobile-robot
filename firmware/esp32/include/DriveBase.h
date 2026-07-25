#pragma once

#include "MotorDriver.h"

/**
 * @brief Controls the differential drive base of the Zumo 2040.
 *
 * Owns both motors and exposes simple directional commands.
 * Uses int32_t to match the Pololu motor API.
 */
class DriveBase
{
public:
    DriveBase();
};