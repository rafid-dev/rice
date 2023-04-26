#include "tt.h"

/*void prefetch(const void* addr) {
#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	_mm_prefetch((char*)addr, _MM_HINT_T0);
#  else
	__builtin_prefetch(addr);
#  endif
}*/

void TranspositionTable::Initialize(int MB)
{
    currentAge = 0;
    this->entries.resize((MB * 1024 * 1024) / sizeof(TTEntry), TTEntry());
    std::fill(entries.begin(), entries.end(), TTEntry());

    //std::cout << "Transposition Table Initialized with " << entries.size() << " entries (" << MB << "MB)\n";
}

void TranspositionTable::storeEntry(U64 key, uint8_t f, Move move, uint8_t depth, int16_t score, int16_t eval, int ply, bool pv)
{
    TTEntry& entry = entries[reduce_hash(key, entries.size())];

    bool replace = false;

    if (entry.key == 0ULL)
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

    if (move != NO_MOVE || key != entry.key)
    {
        entry.move = move;
    }

    if (f == HFEXACT || key != entry.key || depth + 7 + 2 * pv > entry.depth - 4)
    {
        entry.key = key;
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

    ttHit = (entry.key == key);

    return entry;
}

void TranspositionTable::prefetchTT(const U64 key){
    prefetch(&(entries[reduce_hash(key, entries.size())]));
}

void TranspositionTable::clear()
{
    currentAge = 0;
    entries.clear();
}