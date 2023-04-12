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
    TTEntry e;
    std::fill(entries.begin(), entries.end(), e);

    //std::cout << "Transposition Table Initialized with " << entries.size() << " entries (" << MB << "MB)\n";
}

void TranspositionTable::storeEntry(U64 key, int f, Move move, int depth, int score, int eval, int ply, bool pv)
{
    int index = reduce_hash(key, entries.size());

    bool replace = false;

    if (entries[index].key == 0ULL)
    {
        replace = true;
    }
    else
    {
        if (entries[index].age < currentAge || entries[index].depth <= depth){
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

    // if (move != NO_MOVE || key != entries[index].key)
    // {
    //     entries[index].move = move;
    // }

    // if (f == HFEXACT || key != entries[index].key || depth + 7 + 2 * pv > entries[index].depth - 4)
    // {
        entries[index].key = key;
        entries[index].flag = f;
        entries[index].move = move;
        entries[index].depth = depth;
        entries[index].score = score;
        entries[index].eval = eval;
        entries[index].age = currentAge;
        entries[index].pv = pv;
    //}

}

bool TranspositionTable::probeEntry(U64 key, TTEntry *entry, int ply)
{
    int index = reduce_hash(key, entries.size());

    *entry = entries[index];

    if (entry->score > ISMATE)
        entry->score -= ply;
    else if (entry->score < -ISMATE)
        entry->score += ply;

    return (entry->key == key);
}

void TranspositionTable::prefetchTT(const U64 key){
    prefetch(&(entries[reduce_hash(key, entries.size())]));
}

void TranspositionTable::clear()
{
    currentAge = 0;
    entries.clear();
}