#pragma once

template <class T, class Compare>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::Combine(const ISet<T>& other, Operation operation) const {
    BinaryTreeSet result(tree.GetCompare());
    auto enumerator = GetEnumerator();
    while (enumerator->MoveNext()) {
        const T& value = enumerator->CurrentRef();
        if (operation == Operation::Intersection && !other.Contains(value)) continue;
        if (operation == Operation::Difference && other.Contains(value)) continue;
        result.Add(value);
    }
    if (operation == Operation::Union) {
        enumerator = other.GetEnumerator();
        while (enumerator->MoveNext()) result.Add(enumerator->CurrentRef());
    }
    return result;
}

template <class T, class Compare>
BinaryTreeSet<T, Compare>::BinaryTreeSet(Compare compare) : tree(Move(compare)) {}

template <class T, class Compare>
int BinaryTreeSet<T, Compare>::GetCount() const { return tree.GetCount(); }

template <class T, class Compare>
bool BinaryTreeSet<T, Compare>::Add(const T& value) { return tree.Add(value); }

template <class T, class Compare>
bool BinaryTreeSet<T, Compare>::Remove(const T& value) { return tree.Remove(value); }

template <class T, class Compare>
bool BinaryTreeSet<T, Compare>::Contains(const T& value) const { return tree.Find(value) != nullptr; }

template <class T, class Compare>
bool BinaryTreeSet<T, Compare>::IsValid() const { return tree.IsValid(); }

template <class T, class Compare>
int BinaryTreeSet<T, Compare>::GetHeight() const { return tree.GetHeight(); }

template <class T, class Compare>
void BinaryTreeSet<T, Compare>::Clear() { tree.Clear(); }

template <class T, class Compare>
UniquePtr<ISet<T>> BinaryTreeSet<T, Compare>::Union(const ISet<T>& other) const {
    return MakeUnique<BinaryTreeSet>(Combine(other, Operation::Union));
}

template <class T, class Compare>
UniquePtr<ISet<T>> BinaryTreeSet<T, Compare>::Intersection(const ISet<T>& other) const {
    return MakeUnique<BinaryTreeSet>(Combine(other, Operation::Intersection));
}

template <class T, class Compare>
UniquePtr<ISet<T>> BinaryTreeSet<T, Compare>::Difference(const ISet<T>& other) const {
    return MakeUnique<BinaryTreeSet>(Combine(other, Operation::Difference));
}

template <class T, class Compare>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::operator|(const ISet<T>& other) const { return Combine(other, Operation::Union); }

template <class T, class Compare>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::operator&(const ISet<T>& other) const { return Combine(other, Operation::Intersection); }

template <class T, class Compare>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::operator-(const ISet<T>& other) const { return Combine(other, Operation::Difference); }

template <class T, class Compare>
bool BinaryTreeSet<T, Compare>::operator<=(const ISet<T>& other) const { return this->IsSubsetOf(other); }

template <class T, class Compare>
bool BinaryTreeSet<T, Compare>::operator==(const ISet<T>& other) const { return this->Equals(other); }

template <class T, class Compare>
bool BinaryTreeSet<T, Compare>::operator!=(const ISet<T>& other) const { return !this->Equals(other); }

template <class T, class Compare>
template <class Transform>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::Map(Transform transform) const {
    BinaryTreeSet result(tree.GetCompare());
    result.tree = tree.Map(transform);
    return result;
}

template <class T, class Compare>
template <class Predicate>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::Where(Predicate predicate) const {
    BinaryTreeSet result(tree.GetCompare());
    result.tree = tree.Where(predicate);
    return result;
}

template <class T, class Compare>
template <class Accumulator, class Function>
Accumulator BinaryTreeSet<T, Compare>::Reduce(Accumulator initial, Function function) const {
    return tree.Reduce(Move(initial), function);
}

template <class T, class Compare>
template <class Writer>
std::string BinaryTreeSet<T, Compare>::ToString(Writer write) const { return tree.ToString(write); }

template <class T, class Compare>
std::string BinaryTreeSet<T, Compare>::ToString() const { return tree.ToString(); }

template <class T, class Compare>
template <class Reader>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::FromString(const std::string& text, Reader read, Compare compare) {
    BinaryTreeSet result(compare);
    result.tree = BinaryTree<T, Compare>::FromString(text, read, Move(compare));
    return result;
}

template <class T, class Compare>
BinaryTreeSet<T, Compare> BinaryTreeSet<T, Compare>::FromString(const std::string& text, Compare compare) {
    return FromString(text, [](const std::string& value) { return ReadText<T>(value); }, Move(compare));
}

template <class T, class Compare>
UniquePtr<IEnumerator<T>> BinaryTreeSet<T, Compare>::GetEnumerator() const { return tree.GetEnumerator(); }
