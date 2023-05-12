#include "tt.h"

void TranspositionTable::Initialize(int MB)
{
    clear();
    this->entries.resize((MB * 1024 * 1024) / sizeof(TTEntry), TTEntry());
    std::fill(entries.begin(), entries.end(), TTEntry());

    std::cout << "Transposition Table Initialized with " << entries.size() << " entries (" << MB << "MB)\n";
}

void TranspositionTable::storeEntry(U64 key, uint8_t f, Move move, uint8_t depth, int16_t score, int16_t eval, int ply, bool pv)
{
    TTEntry& entry = entries[reduce_hash(key, entries.size())];

    bool replace = false;

    if (entry.key == 0)
    {
        replace = true;
    }
    else
    {
        if (entry.age < currentAge || entry.depth <= depth){
            replace = true;
        }
    }

    if (replace == false){
        return;
    }

    if (score > ISMATE)
        score += ply;
    else if (score < -ISMATE)
        score -= ply;

    if (move != NO_MOVE || static_cast<TTKey>(key) != entry.key)
    {
        entry.move = move;
    }

    if (f == HFEXACT || static_cast<TTKey>(key) != entry.key || depth + 7 + 2 * pv > entry.depth - 4)
    {
        entry.key = static_cast<TTKey>(key);
        entry.flag = f;
        entry.move = move;
        entry.depth = depth;
        entry.score = score;
        entry.eval = eval;
        entry.age = currentAge;
    }
}

TTEntry& TranspositionTable::probeEntry(U64 key, bool& ttHit, int ply)
{
    TTEntry& entry = entries[reduce_hash(key, entries.size())];

    if (entry.score > ISMATE)
        entry.score -= ply;
    else if (entry.score < -ISMATE)
        entry.score += ply;

    ttHit = (static_cast<TTKey>(key) == entry.key);

    return entry;
}

Move TranspositionTable::probeMove(U64 key){
    TTEntry& entry = entries[reduce_hash(key, entries.size())];

    if (static_cast<TTKey>(key) == entry.key){
        return entry.move;
    }

    return NO_MOVE;
}

void TranspositionTable::prefetchTT(const U64 key){
    prefetch(&(entries[reduce_hash(key, entries.size())]));
}

void TranspositionTable::clear()
{
    currentAge = 0;
    entries.clear();
}