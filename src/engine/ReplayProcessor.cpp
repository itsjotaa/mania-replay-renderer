#include "ReplayProcessor.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

// time windows for each judgement (in ms)
// if the difference between hit and note is less than this value -> that judgement
static const long long WINDOW_320  = 16;
static const long long WINDOW_300  = 40;
static const long long WINDOW_200  = 73;
static const long long WINDOW_100  = 103;
static const long long WINDOW_50   = 127;
static const long long WINDOW_MISS = 161;

// converts a time difference to a judgement
// diff = |hit_time - note_time| in ms
static Judgement getJudgement(long long diff) {
    if (diff <= WINDOW_320) return Judgement::J320;
    if (diff <= WINDOW_300) return Judgement::J300;
    if (diff <= WINDOW_200) return Judgement::J200;
    if (diff <= WINDOW_100) return Judgement::J100;
    if (diff <= WINDOW_50)  return Judgement::J50;
    return Judgement::MISS;
}

// checks if column 'col' is active in the bitmask 'keys'
// col ranges from 0 to 3
// example: col=2, keys=0110 -> (0110 >> 2) & 1 = 1 -> active
static bool isColumnActive(int keys, int col) {
    return ((keys >> col) & 1) != 0;
}

std::vector<ProcessedNote> processReplay(
    const BeatmapData& beatmap,
    const ReplayData& replay
) {
    // initialize all notes as MISS with no hit
    std::vector<ProcessedNote> result;
    for (const auto& note : beatmap.notes) {
        result.push_back({note, Judgement::MISS, -1});
    }

    // track which notes have already been matched per column
    // prevents a keypress from matching two notes at once
    // example: if notes[0] and notes[1] are in col=0 and notes[0] was hit,
    // the next keypress in col=0 can only match notes[1] onwards
    std::vector<int> nextUnmatched(beatmap.keyCount, 0);

    // iterate all replay frames looking for new keypresses
    int prevKeys = 0; // key state from the previous frame

    long long firstNoteTime = result.empty() ? 0 : result[0].note.startTime;

    for (const auto& frame : replay.frames) {
        int currKeys = frame.keys;

        // skip frames that occur before the map starts
        // (these are inputs from the osu! menu)
        if (frame.timestamp < firstNoteTime - WINDOW_MISS) {
            prevKeys = currKeys; // still update to avoid accumulating diffs
            continue;
        }

        // detect which columns were pressed in this frame
        // XOR finds bits that changed
        // AND with currKeys keeps only bits that went from 0 to 1
        int newlyPressed = (currKeys ^ prevKeys) & currKeys;

        // for each possible column (0 to keyCount-1)
        for (int col = 0; col < beatmap.keyCount; col++) {

            // check if this specific column was just pressed
            if (!isColumnActive(newlyPressed, col)) continue;

            // find the closest unmatched note in this column
            // start from nextUnmatched[col] to skip already matched notes
            int bestIdx = -1;
            long long bestDiff = WINDOW_MISS + 1; // worse than miss

            for (int i = nextUnmatched[col]; i < (int)result.size(); i++) {
                // only consider notes in this column
                if (result[i].note.column != col) continue;

                // skip already matched notes
                if (result[i].hitTime != -1) continue;

                long long diff = std::abs(frame.timestamp - result[i].note.startTime);

                // if the note passed by more than WINDOW_MISS, no point searching further
                // the following notes are even further away
                if (frame.timestamp - result[i].note.startTime > WINDOW_MISS) {
                    // advance nextUnmatched so we never check this note again
                    nextUnmatched[col] = i + 1;
                    continue;
                }

                // if the note is too far in the future, stop
                if (result[i].note.startTime - frame.timestamp > WINDOW_MISS) break;

                // this note is within the window -> it is a candidate
                if (diff < bestDiff) {
                    bestDiff = diff;
                    bestIdx  = i;
                }
            }

            // if a candidate was found, assign the hit
            if (bestIdx != -1) {
                result[bestIdx].judgement = getJudgement(bestDiff);
                result[bestIdx].hitTime   = frame.timestamp;
            }
            // if none found -> keypress with no note (has no effect)
        }

        prevKeys = currKeys; // update previous key state
    }

    // notes that still have hitTime == -1 are already marked as MISS
    return result;
}
