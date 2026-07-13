#pragma once

// Starts LoRaWAN initialization and network joining in a background task.
// Returns false only when the background task could not be created.
bool lorawanServiceStart();
