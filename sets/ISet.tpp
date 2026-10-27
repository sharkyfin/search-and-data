#pragma once

template <class T>
bool ISet<T>::IsSubsetOf(const ISet<T>& other) const {
    if (this->GetCount() > other.GetCount()) return false;
    auto enumerator = this->GetEnumerator();
    while (enumerator->MoveNext()) {
        if (!other.Contains(enumerator->CurrentRef())) return false;
    }
    return true;
}

template <class T>
bool ISet<T>::Equals(const ISet<T>& other) const {
    return this->GetCount() == other.GetCount() && IsSubsetOf(other);
}
