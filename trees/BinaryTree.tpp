#pragma once

template <class T, class Compare>
BinaryTree<T, Compare>::Node::Node(T val) : value(Move(val)) {}

template <class T, class Compare>
void BinaryTree<T, Compare>::Enumerator::CheckVersion() const {
    if (version != owner.version) throw std::logic_error("Tree changed during enumeration.");
}

template <class T, class Compare>
BinaryTree<T, Compare>::Enumerator::Enumerator(const BinaryTree& owner, Traversal order) : owner(owner), version(owner.version), order(order) {
    if (owner.root) pending.PushFront(Frame{owner.root.get(), 0});
    if (static_cast<int>(order) < 0 || static_cast<int>(order) > 6) {
        throw std::invalid_argument("Invalid traversal.");
    }
}

template <class T, class Compare>
bool BinaryTree<T, Compare>::Enumerator::MoveNext() {
    CheckVersion();
    current = nullptr;
    if (order == Traversal::BreadthFirst) {
        if (pending.IsEmpty()) return false;
        current = (*pending.begin()).node;
        pending.PopFront();
        if (current->left) pending.PushBack(Frame{current->left.get(), 0});
        if (current->right) pending.PushBack(Frame{current->right.get(), 0});
        return true;
    }
    const char* steps[]{"KLP", "KPL", "LPK", "LKP", "PLK", "PKL"};
    while (!pending.IsEmpty()) {
        Frame& frame = *pending.begin();
        if (frame.stage == 3) { pending.PopFront(); continue; }
        const char step = steps[static_cast<int>(order)][frame.stage++];
        if (step == 'K') { current = frame.node; return true; }
        const Node* child = step == 'L' ? frame.node->left.get() : frame.node->right.get();
        if (child) pending.PushFront(Frame{child, 0});
    }
    return false;
}

template <class T, class Compare>
const T& BinaryTree<T, Compare>::Enumerator::CurrentRef() const {
    CheckVersion();
    if (!current) throw std::logic_error("Enumerator has no current element.");
    return current->value;
}

template <class T, class Compare>
int BinaryTree<T, Compare>::Height(const UniquePtr<Node>& node) noexcept {
    return node ? node->height : 0;
}

template <class T, class Compare>
void BinaryTree<T, Compare>::Update(Node& node) noexcept {
    const int lh = Height(node.left);
    const int rh = Height(node.right);
    node.height = 1 + (lh > rh ? lh : rh);
}

template <class T, class Compare>
void BinaryTree<T, Compare>::RotateLeft(UniquePtr<Node>& node) {
    auto next = Move(node->right);
    node->right = Move(next->left);
    Update(*node);
    next->left = Move(node);
    Update(*next);
    node = Move(next);
}

template <class T, class Compare>
void BinaryTree<T, Compare>::RotateRight(UniquePtr<Node>& node) {
    auto next = Move(node->left);
    node->left = Move(next->right);
    Update(*node);
    next->right = Move(node);
    Update(*next);
    node = Move(next);
}

template <class T, class Compare>
void BinaryTree<T, Compare>::Balance(UniquePtr<Node>& node) {
    if (!node) return;
    Update(*node);
    const int difference = Height(node->left) - Height(node->right);
    if (difference > 1) {
        if (Height(node->left->left) < Height(node->left->right)) RotateLeft(node->left);
        RotateRight(node);
    } else if (difference < -1) {
        if (Height(node->right->right) < Height(node->right->left)) RotateRight(node->right);
        RotateLeft(node);
    }
}

template <class T, class Compare>
void BinaryTree<T, Compare>::Invalidate() noexcept {
    ++version;
}

template <class T, class Compare>
template <class U>
bool BinaryTree<T, Compare>::Insert(UniquePtr<Node>& node, U&& value) {
    if (!node) {
        node = MakeUnique<Node>(static_cast<U&&>(value));
        return true;
    }
    bool inserted = false;
    if (compare(value, node->value)) inserted = Insert(node->left, static_cast<U&&>(value));
    else if (allowDuplicates || compare(node->value, value)) {
        inserted = Insert(node->right, static_cast<U&&>(value));
    } else return false;
    Balance(node);
    return inserted;
}

template <class T, class Compare>
UniquePtr<typename BinaryTree<T, Compare>::Node> BinaryTree<T, Compare>::ExtractMinimum(UniquePtr<Node>& node) {
    if (!node->left) {
        auto minimum = Move(node);
        node = Move(minimum->right);
        return minimum;
    }
    auto minimum = ExtractMinimum(node->left);
    Balance(node);
    return minimum;
}

template <class T, class Compare>
UniquePtr<typename BinaryTree<T, Compare>::Node> BinaryTree<T, Compare>::ExtractMaximum(UniquePtr<Node>& node) {
    if (!node->right) {
        auto maximum = Move(node);
        node = Move(maximum->left);
        return maximum;
    }
    auto maximum = ExtractMaximum(node->right);
    Balance(node);
    return maximum;
}

template <class T, class Compare>
template <class Key>
bool BinaryTree<T, Compare>::Erase(UniquePtr<Node>& node, const Key& key) {
    if (!node) return false;
    bool removed = true;
    if (compare(key, node->value)) removed = Erase(node->left, key);
    else if (compare(node->value, key)) removed = Erase(node->right, key);
    else if (!node->left) {
        auto next = Move(node->right);
        node = Move(next);
    } else if (!node->right) {
        auto next = Move(node->left);
        node = Move(next);
    } else {
        auto next = ExtractMinimum(node->right);
        next->left = Move(node->left);
        next->right = Move(node->right);
        node = Move(next);
    }
    Balance(node);
    return removed;
}

template <class T, class Compare>
template <class Key>
typename BinaryTree<T, Compare>::Node* BinaryTree<T, Compare>::FindNode(const Key& key) const {
    Node* node = root.get();
    while (node) {
        if (compare(key, node->value)) node = node->left.get();
        else if (compare(node->value, key)) node = node->right.get();
        else return node;
    }
    return nullptr;
}

template <class T, class Compare>
UniquePtr<typename BinaryTree<T, Compare>::Node> BinaryTree<T, Compare>::Clone(const Node* node, int& copied) {
    if (!node) return {};
    auto result = MakeUnique<Node>(node->value);
    result->left = Clone(node->left.get(), copied);
    result->right = Clone(node->right.get(), copied);
    result->height = node->height;
    ++copied;
    return result;
}

template <class T, class Compare>
int BinaryTree<T, Compare>::Check(const Node* node, const T* lower, const T* upper, int& visited) const {
    if (!node) return 0;
    if (lower && (compare(node->value, *lower) || (!allowDuplicates && !compare(*lower, node->value)))) return -1;
    if (upper && (compare(*upper, node->value) || (!allowDuplicates && !compare(node->value, *upper)))) return -1;
    ++visited;
    const int left = Check(node->left.get(), lower, &node->value, visited);
    const int right = Check(node->right.get(), &node->value, upper, visited);
    const int diff = left > right ? left - right : right - left;
    const int maxH = left > right ? left : right;
    if (left < 0 || right < 0 || diff > 1 || node->height != 1 + maxH) return -1;
    return node->height;
}

template <class T, class Compare>
bool BinaryTree<T, Compare>::Same(const Node* first, const Node* second) const {
    if (!first || !second) return first == second;
    return !compare(first->value, second->value) && !compare(second->value, first->value)
        && Same(first->left.get(), second->left.get()) && Same(first->right.get(), second->right.get());
}

template <class T, class Compare>
bool BinaryTree<T, Compare>::ContainsNode(const Node* node, const Node* candidate) const {
    if (!candidate) return true;
    if (!node) return false;
    if (compare(candidate->value, node->value)) return ContainsNode(node->left.get(), candidate);
    if (compare(node->value, candidate->value)) return ContainsNode(node->right.get(), candidate);
    return Same(node, candidate) || (allowDuplicates
        && (ContainsNode(node->left.get(), candidate) || ContainsNode(node->right.get(), candidate)));
}

template <class T, class Compare>
const T& BinaryTree<T, Compare>::Follow(const Node* node, const std::string& path) {
    for (char step : path) {
        if (step != 'L' && step != 'R') throw std::invalid_argument("Use L and R in paths.");
        if (!node) throw std::out_of_range("Path does not exist.");
        node = step == 'L' ? node->left.get() : node->right.get();
    }
    if (!node) throw std::out_of_range("Path does not exist.");
    return node->value;
}

template <class T, class Compare>
BinaryTree<T, Compare>::BinaryTree(Compare compare, bool allowDuplicates) : compare(Move(compare)), allowDuplicates(allowDuplicates) {}

template <class T, class Compare>
BinaryTree<T, Compare>::BinaryTree(const BinaryTree& other) : compare(other.compare), allowDuplicates(other.allowDuplicates) {
    root = Clone(other.root.get(), count);
}

template <class T, class Compare>
BinaryTree<T, Compare>::BinaryTree(BinaryTree&& other) : root(Move(other.root)), count(other.count), compare(Move(other.compare)), allowDuplicates(other.allowDuplicates) {
    other.count = 0;
    other.Invalidate();
}

template <class T, class Compare>
BinaryTree<T, Compare>& BinaryTree<T, Compare>::operator=(const BinaryTree& other) {
    if (this != &other) {
        BinaryTree copy(other);
        Swap(copy);
    }
    return *this;
}

template <class T, class Compare>
BinaryTree<T, Compare>& BinaryTree<T, Compare>::operator=(BinaryTree&& other) {
    if (this != &other) {
        BinaryTree moved(Move(other));
        Swap(moved);
    }
    return *this;
}

template <class T, class Compare>
void BinaryTree<T, Compare>::Swap(BinaryTree& other) {
    ::Swap(compare, other.compare);
    root.swap(other.root);
    ::Swap(count, other.count);
    ::Swap(allowDuplicates, other.allowDuplicates);
    Invalidate();
    other.Invalidate();
}

template <class T, class Compare>
int BinaryTree<T, Compare>::GetCount() const { return count; }

template <class T, class Compare>
int BinaryTree<T, Compare>::GetHeight() const noexcept { return Height(root); }

template <class T, class Compare>
const Compare& BinaryTree<T, Compare>::GetCompare() const noexcept { return compare; }

template <class T, class Compare>
bool BinaryTree<T, Compare>::IsValid() const {
    int visited = 0;
    return Check(root.get(), nullptr, nullptr, visited) >= 0 && visited == count;
}

template <class T, class Compare>
template <class U>
bool BinaryTree<T, Compare>::Add(U&& value) {
    if (!Insert(root, static_cast<U&&>(value))) return false;
    ++count;
    Invalidate();
    return true;
}

template <class T, class Compare>
template <class Key>
bool BinaryTree<T, Compare>::Remove(const Key& key) {
    if (!Erase(root, key)) return false;
    --count;
    Invalidate();
    return true;
}

template <class T, class Compare>
template <class Key>
T* BinaryTree<T, Compare>::Find(const Key& key) {
    Node* node = FindNode(key);
    return node ? &node->value : nullptr;
}

template <class T, class Compare>
template <class Key>
const T* BinaryTree<T, Compare>::Find(const Key& key) const {
    const Node* node = FindNode(key);
    return node ? &node->value : nullptr;
}

template <class T, class Compare>
void BinaryTree<T, Compare>::Clear() noexcept {
    Invalidate();
    root.reset();
    count = 0;
}

template <class T, class Compare>
const T& BinaryTree<T, Compare>::Maximum() const {
    if (!root) throw std::out_of_range("Tree is empty.");
    const Node* node = root.get();
    while (node->right) node = node->right.get();
    return node->value;
}

template <class T, class Compare>
T BinaryTree<T, Compare>::PopMaximum() {
    if (!root) throw std::out_of_range("Tree is empty.");
    auto maximum = ExtractMaximum(root);
    --count;
    Invalidate();
    return Move(maximum->value);
}

template <class T, class Compare>
UniquePtr<IEnumerator<T>> BinaryTree<T, Compare>::GetEnumerator() const {
    return GetEnumerator(Traversal::LKP);
}

template <class T, class Compare>
UniquePtr<IEnumerator<T>> BinaryTree<T, Compare>::GetEnumerator(Traversal order) const {
    return MakeUnique<Enumerator>(*this, order);
}

template <class T, class Compare>
template <class Transform>
BinaryTree<T, Compare> BinaryTree<T, Compare>::Map(Transform transform, Traversal order) const {
    BinaryTree result(compare, allowDuplicates);
    auto enumerator = GetEnumerator(order);
    while (enumerator->MoveNext()) result.Add(transform(enumerator->CurrentRef()));
    return result;
}

template <class T, class Compare>
template <class Predicate>
BinaryTree<T, Compare> BinaryTree<T, Compare>::Where(Predicate predicate, Traversal order) const {
    BinaryTree result(compare, allowDuplicates);
    auto enumerator = GetEnumerator(order);
    while (enumerator->MoveNext()) {
        const T& value = enumerator->CurrentRef();
        if (predicate(value)) result.Add(value);
    }
    return result;
}

template <class T, class Compare>
template <class Accumulator, class Function>
Accumulator BinaryTree<T, Compare>::Reduce(Accumulator initial, Function function, Traversal order) const {
    auto enumerator = GetEnumerator(order);
    while (enumerator->MoveNext()) initial = function(enumerator->CurrentRef(), Move(initial));
    return initial;
}

template <class T, class Compare>
BinaryTree<T, Compare> BinaryTree<T, Compare>::Merge(const IEnumerable<T>& other) const {
    BinaryTree result(*this);
    auto enumerator = other.GetEnumerator();
    while (enumerator->MoveNext()) result.Add(enumerator->CurrentRef());
    return result;
}

template <class T, class Compare>
BinaryTree<T, Compare> BinaryTree<T, Compare>::ExtractSubtree(const T& value) const {
    const Node* node = FindNode(value);
    if (!node) throw std::out_of_range("Subtree root not found.");
    BinaryTree result(compare, allowDuplicates);
    result.root = Clone(node, result.count);
    return result;
}

template <class T, class Compare>
bool BinaryTree<T, Compare>::ContainsSubtree(const BinaryTree& other) const {
    return ContainsNode(root.get(), other.root.get());
}

template <class T, class Compare>
const T& BinaryTree<T, Compare>::GetByPath(const std::string& path) const { return Follow(root.get(), path); }

template <class T, class Compare>
const T& BinaryTree<T, Compare>::GetByPath(const T& start, const std::string& path) const { return Follow(FindNode(start), path); }

template <class T, class Compare>
template <class Writer>
std::string BinaryTree<T, Compare>::ToString(Writer write) const {
    return WriteValues(GetEnumerator(), write);
}

template <class T, class Compare>
std::string BinaryTree<T, Compare>::ToString() const {
    return ToString([](const T& value) { return WriteText(value); });
}

template <class T, class Compare>
template <class Reader>
BinaryTree<T, Compare> BinaryTree<T, Compare>::FromString(const std::string& text, Reader read,
        Compare compare, bool allowDuplicates) {
    BinaryTree result(Move(compare), allowDuplicates);
    std::istringstream input(text);
    std::string value;
    while (input >> std::ws && !input.eof()) {
        if (!(input >> std::quoted(value))) throw std::invalid_argument("Invalid value.");
        result.Add(read(value));
    }
    return result;
}

template <class T, class Compare>
BinaryTree<T, Compare> BinaryTree<T, Compare>::FromString(const std::string& text, Compare compare, bool allowDuplicates) {
    return FromString(text, [](const std::string& value) { return ReadText<T>(value); }, Move(compare), allowDuplicates);
}
