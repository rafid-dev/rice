#include "tt.h"

void TranspositionTable::Initialize(int MB)
{
    this->entries.resize((MB * 1024 * 1024) / sizeof(TTEntry), TTEntry());
    TTEntry e;
    std::fill(entries.begin(), entries.end(), e);

    std::cout << "Transposition Table Initialized with " << entries.size() << " entries (" << MB << "MB)\n";
}

void TranspositionTable::storeEntry(U64 key, int f, Move move, int depth, int score, int eval, int ply)
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

    entries[index].key = key;
    entries[index].flag = f;
    entries[index].move = move;
    entries[index].depth = depth;
    entries[index].score = score;
    entries[index].eval = eval;
    entries[index].age = currentAge;
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

void TranspositionTable::clear()
{
    currentAge = 0;
    entries.clear();
}