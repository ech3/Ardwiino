#ifdef INPUT_WII
    uint8_t *wiiData;
    // If we didn't send the last packet, then we need to wait some time as the wii controllers do not like being polled quickly
    if (micros() - lastTick > 800) {
        lastTick = micros();
        wiiData = tickWii();
    } else {
        wiiData = lastSuccessfulWiiPacket;
    }
    bool wiiValid = wiiData != NULL;
    lastWiiWasSuccessful = wiiValid;
    uint8_t wiiButtonsLow, wiiButtonsHigh, vel, which, lastTapWiiGh5, lastTapWii = 0;
    uint16_t accX, accY, accZ = 0;
    if (wiiValid) {
        memcpy(lastSuccessfulWiiPacket, wiiData, sizeof(lastSuccessfulWiiPacket));
        wiiButtonsLow = ~wiiData[4];
        wiiButtonsHigh = ~wiiData[5];
        if (hiRes) {
            wiiButtonsLow = ~wiiData[6];
            wiiButtonsHigh = ~wiiData[7];
        }
#ifdef INPUT_WII_TAP
        lastTapWii = (wiiData[2] & 0x1f);
        // GH3 guitars set this bit, while WT and GH5 guitars do not
        if ((wiiData[0] & (1 << 7)) != 0) {
            lastTapWii = 0;
        }
        if (lastTapWii == 0x0f) {
            lastTapWiiGh5 = 0;
        } else if (lastTapWii == 0x04) {
            lastTapWiiGh5 = 0x95;
        } else if (lastTapWii == 0x07) {
            lastTapWiiGh5 = 0xB0;
        } else if (lastTapWii == 0x0A) {
            lastTapWiiGh5 = 0xCD;
        } else if (lastTapWii == 0x0C || lastTapWii == 0x0D) {
            lastTapWiiGh5 = 0xE6;
        } else if (lastTapWii == 0x12 || lastTapWii == 0x13) {
            lastTapWiiGh5 = 0x1A;
        } else if (lastTapWii == 0x14 || lastTapWii == 0x15) {
            lastTapWiiGh5 = 0x2F;
        } else if (lastTapWii == 0x17 || lastTapWii == 0x18) {
            lastTapWiiGh5 = 0x49;
        } else if (lastTapWii == 0x1A) {
            lastTapWiiGh5 = 0x66;
        } else if (lastTapWii == 0x1F) {
            lastTapWiiGh5 = 0x7F;
        }

#endif
#ifdef INPUT_WII_DRUM
        vel = (7 - (wiiData[3] >> 5)) << 5;
        which = (wiiData[2] & 0b01111100) >> 1;
        switch (which) {
            case 0x1B:
                drumVelocity[DRUM_KICK] = vel;
                break;
            case 0x12:
                drumVelocity[DRUM_GREEN] = vel;
                break;
            case 0x19:
                drumVelocity[DRUM_RED] = vel;
                break;
            case 0x11:
                drumVelocity[DRUM_YELLOW] = vel;
                break;
            case 0x0F:
                drumVelocity[DRUM_BLUE] = vel;
                break;
            case 0x0E:
                drumVelocity[DRUM_ORANGE] = vel;
                break;
        }
#endif
#ifdef INPUT_WII_NUNCHUK
        accX = ((wiiData[2] << 2) | ((wiiData[5] & 0xC0) >> 6)) - 511;
        accY = ((wiiData[3] << 2) | ((wiiData[5] & 0x30) >> 4)) - 511;
        accZ = ((wiiData[4] << 2) | ((wiiData[5] & 0xC) >> 2)) - 511;
#endif
    }
#endif