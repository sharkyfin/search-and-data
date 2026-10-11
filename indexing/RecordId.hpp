#pragma once

struct RecordId {
    int position;

    bool operator==(const RecordId& other) const {
        return position == other.position;
    }
};
