#pragma once

#include <lightwave/core.hpp>
#include <lightwave/math.hpp>
#include <lightwave/shape.hpp>

#include <numeric>

namespace lightwave {

/**
 * @brief Parent class for shapes that combine many individual shapes (e.g.,
 * triangle meshes), and hence benefit from building an acceleration structure
 * over their children.
 *
 * To use this class, you will need to implement the following methods:
 * - numberOfPrimitives()           -- report the number of individual children
 * that the shape has
 * - intersect(primitiveIndex, ...) -- intersect a single child (identified by
 * the given index) for the given ray
 * - getBoundingBox(primitiveIndex) -- return the bounding box of a single child
 * (used for building the BVH)
 * - getCentroid(primitiveIndex)    -- return the centroid of a single child
 * (used for building the BVH)
 *
 * @example For a simple example of how to use this class, look at @ref
 * shapes/group.cpp
 * @see Group
 * @see TriangleMesh
 */
class AccelerationStructure : public Shape {
    /// @brief The datatype used to index BVH nodes and the primitive index
    /// remapping.
    typedef int32_t NodeIndex;

    /// @brief A node in our binary BVH tree.
    struct Node {
        /// @brief The axis aligned bounding box of this node.
        Bounds aabb;
        /**
         * @brief Either the index of the left child node in m_nodes (for
         * internal nodes), or the first primitive in m_primitiveIndices (for
         * leaf nodes).
         * @note For efficiency, we store the BVH nodes so that the right child
         * always directly follows the left child, i.e., the index of the right
         * child is always @code leftFirst + 1 @endcode .
         * @note For efficiency, we store primitives so that children of a leaf
         * node are always contigous in m_primitiveIndices.
         */
        NodeIndex leftFirst;
        /// @brief The number of primitives in a leaf node, or 0 to indicate
        /// that this node is not a leaf node.
        NodeIndex primitiveCount;

        /// @brief Whether this BVH node is a leaf node.
        bool isLeaf() const { return primitiveCount != 0; }

        /// @brief For internal nodes: The index of the left child node in
        /// m_nodes.
        NodeIndex leftChildIndex() const { return leftFirst; }
        /// @brief For internal nodes: The index of the right child node in
        /// m_nodes.
        NodeIndex rightChildIndex() const { return leftFirst + 1; }

        /// @brief For leaf nodes: The first index in m_primitiveIndices.
        NodeIndex firstPrimitiveIndex() const { return leftFirst; }
        /// @brief For leaf nodes: The last index in m_primitiveIndices (still
        /// included).
        NodeIndex lastPrimitiveIndex() const {
            return leftFirst + primitiveCount - 1;
        }
    };

    /// @brief A list of all BVH nodes.
    std::vector<Node> m_nodes;
    /**
     * @brief Mapping from internal @c NodeIndex to @c primitiveIndex as used by
     * all interface methods. For efficient storage, we assume that children of
     * BVH leaf nodes have contiguous indices, which would require re-ordering
     * the primitives. For simplicity, we instead perform this re-ordering on a
     * list of indices (which starts of as @code 0, 1, 2, ..., primitiveCount -
     * 1 @endcode ), which allows us to translate from re-ordered (contiguous)
     * indices to the indices the user of this class expects.
     */
    std::vector<int> m_primitiveIndices;

    /// @brief Returns the root BVH node.
    const Node &rootNode() const {
        // by convention, this is always the first element of m_nodes
        return m_nodes.front();
    }

    /**
     * @brief Intersects a BVH node, recursing into children (for internal
     * nodes), or intersecting all primitives (for leaf nodes).
     */
    bool intersectNode(const Node &node, const Ray &ray, Intersection &its,
                       Sampler &rng) const {
        // update the statistic tracking how many BVH nodes have been tested for
        // intersection
        its.stats.bvhCounter++;

        bool wasIntersected = false;
        if (node.isLeaf()) {
            for (NodeIndex i = 0; i < node.primitiveCount; i++) {
                // update the statistic tracking how many children have been
                // tested for intersection
                its.stats.primCounter++;
                // test the child for intersection
                wasIntersected |= intersect(
                    m_primitiveIndices[node.leftFirst + i], ray, its, rng);
            }
        } else { // internal node
            // test which bounding box is intersected first by the ray.
            // this allows us to traverse the children in the order they are
            // intersected in, which can help prune a lot of unnecessary
            // intersection tests.
            const auto leftT =
                intersectAABB(m_nodes[node.leftChildIndex()].aabb, ray);
            const auto rightT =
                intersectAABB(m_nodes[node.rightChildIndex()].aabb, ray);
            if (leftT < rightT) { // left child is hit first; test left child
                                  // first, then right child
                if (leftT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.leftChildIndex()], ray, its, rng);
                if (rightT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.rightChildIndex()], ray, its, rng);
            } else { // right child is hit first; test right child first, then
                     // left child
                if (rightT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.rightChildIndex()], ray, its, rng);
                if (leftT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.leftChildIndex()], ray, its, rng);
            }
        }
        return wasIntersected;
    }

    /// @brief Performs a slab test to intersect a bounding box with a ray,
    /// returning Infinity in case the ray misses.
    float intersectAABB(const Bounds &bounds, const Ray &ray) const {
        const auto t1 = (bounds.min() - ray.origin) / ray.direction;
        // intersect all axes at once with the maximum slabs of the bounding box
        const auto t2 = (bounds.max() - ray.origin) / ray.direction;

        // the elementwiseMin picks the near slab for each axis, of which we
        // then take the maximum
        const auto tNear = elementwiseMin(t1, t2).maxComponent();
        // the elementwiseMax picks the far slab for each axis, of which we then
        // take the minimum
        const auto tFar = elementwiseMax(t1, t2).minComponent();

        if (tFar < tNear)
            return Infinity; // the ray does not intersect the bounding box
        if (tFar < Epsilon)
            return Infinity; // the bounding box lies behind the ray origin

        return tNear; // return the first intersection with the bounding box
                      // (may also be negative!)
    }

    /// @brief Computes the axis aligned bounding box for a leaf BVH node
    void computeAABB(Node &node) {
        node.aabb = Bounds::empty();
        for (NodeIndex i = 0; i < node.primitiveCount; i++) {
            const Bounds childAABB =
                getBoundingBox(m_primitiveIndices[node.leftFirst + i]);
            node.aabb.extend(childAABB); // extend the bounding box
        }
    }

    /// @brief Computes the surface area of a bounding box.
    float surfaceArea(const Bounds &bounds) const {
        const auto size = bounds.diagonal();
        return 2 * (size.x() * size.y() + size.x() * size.z() +
                    size.y() * size.z());
    }

    /**
     * For a given node, computes split axis and split position that minimize
     * the surface area heuristic.
     * @param node The BVH node to compute the split for.
     * @param out bestSplitAxis The optimal split axis, or -1 if no useful split
     * exists
     * @param out bestSplitPosition The optimal split position, undefined if no
     * useful split exists
     */
    void binning(const Node &node, int &bestSplitAxis,
                 float &bestSplitPosition) {
        // NOT_IMPLEMENTED        
        constexpr int NumBins = 16; // Number of bins for SAH
        struct Bin {
            Bounds bounds = Bounds::empty();
            int count = 0;
        };

        bestSplitAxis = -1;
        bestSplitPosition = 0;
        float bestCost = Infinity;

        // Check if the bounding box is valid
        if (node.aabb.isEmpty() || node.primitiveCount <= 1) {
            return; // No split is needed for empty or single-primitive nodes
        }

        // Get all centroids
        Bounds all_centroids;
        for (int i = 0; i < node.primitiveCount; ++i){
            int primitiveIndex = m_primitiveIndices[node.leftFirst + i];
            Point centroid = getCentroid(primitiveIndex);
            all_centroids.extend(centroid);
        }

        // Iterate over each axis to find the best split
        for (int axis = 0; axis < Bounds::Dimension; ++axis) {
            // float minCoord = node.aabb.min()[axis];
            // float maxCoord = node.aabb.max()[axis];
            float minCoord = all_centroids.min()[axis];
            float maxCoord = all_centroids.max()[axis];
            float range = maxCoord - minCoord;

            if (range <= Epsilon) {
                continue; // Skip axis with negligible range
            }

            float scale = NumBins / range;
            Bin bins[NumBins] = {};

            // Bin assignment: Group primitives into bins
            for (int i = 0; i < node.primitiveCount; ++i) {
                int primitiveIndex = m_primitiveIndices[node.leftFirst + i];
                if (primitiveIndex < 0 || primitiveIndex >= int(m_primitiveIndices.size())) {
                    continue; // Ignore invalid primitive indices
                }

                Point centroid = getCentroid(primitiveIndex);
                int binIndex = int((centroid[axis] - minCoord) * scale);
                binIndex = clamp(binIndex, 0, NumBins - 1);

                bins[binIndex].count++;
                bins[binIndex].bounds.extend(getBoundingBox(primitiveIndex));
            }

            // Compute cumulative data for SAH evaluation
            Bounds leftBounds[NumBins] = {};
            int leftCounts[NumBins] = {0};
            Bounds rightBounds[NumBins] = {};
            int rightCounts[NumBins] = {0};

            leftBounds[0] = bins[0].bounds;
            leftCounts[0] = bins[0].count;
            for (int i = 1; i < NumBins; ++i) {
                leftBounds[i] = leftBounds[i - 1];
                leftBounds[i].extend(bins[i].bounds);
                leftCounts[i] = leftCounts[i - 1] + bins[i].count;
            }

            rightBounds[NumBins - 1] = bins[NumBins - 1].bounds;
            rightCounts[NumBins - 1] = bins[NumBins - 1].count;
            for (int i = NumBins - 2; i >= 0; --i) {
                rightBounds[i] = rightBounds[i + 1];
                rightBounds[i].extend(bins[i].bounds);
                rightCounts[i] = rightCounts[i + 1] + bins[i].count;
            }

            // Evaluate split cost using SAH
            float totalSurfaceArea = surfaceArea(node.aabb);
            for (int i = 0; i < NumBins - 1; ++i) {
                float leftArea = surfaceArea(leftBounds[i]);
                float rightArea = surfaceArea(rightBounds[i + 1]);

                float splitCost = 0.125f + (leftCounts[i] * leftArea + rightCounts[i + 1] * rightArea) / totalSurfaceArea;

                if (splitCost < bestCost) {
                    bestCost = splitCost;
                    bestSplitAxis = axis;
                    bestSplitPosition = minCoord + (i + 1) / scale;
                }
            }
        }

        // Relax validation: Ensure at least some split is found
        if (bestSplitAxis == -1) {
            std::cerr << "Warning: No valid split found for node. Assigning default split.\n";
            bestSplitAxis = 0; // Default to the first axis
            bestSplitPosition = (node.aabb.min()[0] + node.aabb.max()[0]) / 2.0f;
        }

        } // end code

    /// @brief Attempts to subdivide a given BVH node.
    void subdivide(Node &parent) {
        // only subdivide if enough children are available.
        if (parent.primitiveCount <= 2) {
            return;
        }

        // set to true when implementing binning
        static constexpr bool UseSAH = true;

        int splitAxis = -1;
        float splitPosition;
        if (UseSAH) {
            // pick split axis and position using binned SAH
            binning(parent, splitAxis, splitPosition);
        } else {
            // split in the middle of the longest axis
            splitAxis     = parent.aabb.diagonal().maxComponentIndex();
            splitPosition = parent.aabb.center()[splitAxis];
        }

        if (splitAxis == -1) {
            // a split axis of -1 indicates that no useful split exists
            return;
        }

        // the point at which to split (note that primitives must be re-ordered
        // so that all children of the left node will have a smaller index than
        // firstRightIndex, and nodes on the right will have an index larger or
        // equal to firstRightIndex)
        NodeIndex firstRightIndex = parent.firstPrimitiveIndex();
        NodeIndex lastLeftIndex   = parent.lastPrimitiveIndex();

        // partition algorithm (you might remember this from quicksort)
        while (firstRightIndex <= lastLeftIndex) {
            if (getCentroid(m_primitiveIndices[firstRightIndex])[splitAxis] <
                splitPosition) {
                firstRightIndex++;
            } else {
                std::swap(m_primitiveIndices[firstRightIndex],
                          m_primitiveIndices[lastLeftIndex--]);
            }
        }

        const NodeIndex firstLeftIndex = parent.firstPrimitiveIndex();
        const NodeIndex leftCount      = firstRightIndex - firstLeftIndex;
        const NodeIndex rightCount     = parent.primitiveCount - leftCount;

        if (leftCount == 0 || rightCount == 0) {
            // if either child gets no primitives, we abort subdividing
            return;
        }

        // the two children will always be contiguous in our m_nodes list
        const NodeIndex leftChildIndex  = (NodeIndex) (m_nodes.size() + 0);
        const NodeIndex rightChildIndex = (NodeIndex) (m_nodes.size() + 1);
        parent.primitiveCount = 0; // mark the parent node as internal node
        parent.leftFirst      = leftChildIndex;

        m_nodes.emplace_back();
        m_nodes[leftChildIndex].leftFirst      = firstLeftIndex;
        m_nodes[leftChildIndex].primitiveCount = leftCount;

        m_nodes.emplace_back();
        m_nodes[rightChildIndex].leftFirst      = firstRightIndex;
        m_nodes[rightChildIndex].primitiveCount = rightCount;

        // first, process the left child node (and all of its children)
        computeAABB(m_nodes[leftChildIndex]);
        subdivide(m_nodes[leftChildIndex]);
        // then, process the right child node (and all of its children)
        computeAABB(m_nodes[rightChildIndex]);
        subdivide(m_nodes[rightChildIndex]);
    }

protected:
    /// @brief Returns the number of children (individual shapes) that are part
    /// of this acceleration structure.
    virtual int numberOfPrimitives() const = 0;
    /// @brief Intersect a single child (identified by the index) with the given
    /// ray.
    virtual bool intersect(int primitiveIndex, const Ray &ray,
                           Intersection &its, Sampler &rng) const = 0;
    /// @brief Returns the axis aligned bounding box of the given child.
    virtual Bounds getBoundingBox(int primitiveIndex) const = 0;
    /// @brief Returns the centroid of the given child.
    virtual Point getCentroid(int primitiveIndex) const = 0;

    /// @brief Builds the acceleration structure.
    void buildAccelerationStructure() {
        Timer buildTimer;

        // fill primitive indices with 0 to primitiveCount - 1
        m_primitiveIndices.resize(numberOfPrimitives());
        std::iota(m_primitiveIndices.begin(), m_primitiveIndices.end(), 0);

        // create root node
        auto &root          = m_nodes.emplace_back();
        root.leftFirst      = 0;
        root.primitiveCount = numberOfPrimitives();
        computeAABB(root);
        subdivide(root);

        logger(EInfo,
               "built BVH with %ld nodes for %ld primitives in %.1f ms",
               m_nodes.size(),
               numberOfPrimitives(),
               buildTimer.getElapsedTime() * 1000);
    }

public:
    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        if (m_primitiveIndices.empty())
            return false; // exit early if no children exist
        if (intersectAABB(rootNode().aabb, ray) <
            its.t) // test root bounding box for potential hit
            return intersectNode(rootNode(), ray, its, rng);
        return false;
    }

    Bounds getBoundingBox() const override { return rootNode().aabb; }

    Point getCentroid() const override { return rootNode().aabb.center(); }
};

} // namespace lightwave
