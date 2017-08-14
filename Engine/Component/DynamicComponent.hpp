﻿#pragma once

#include <eigen3/Eigen/Dense>

namespace yuki
{

/**
 * \brief Describe the concept of an object having spatial information,
 * such as position and orientation.
 */
class DynamicComponent
{
    mutable Eigen::Affine3f mLocalToWorld;
    // todo: assert normalized quaternion
    Eigen::Quaternionf mOrientation;
    Eigen::Vector3f mPosition;
    mutable bool mDirtyMatrix = true;

    void _polluteMatrix()
    {
        mDirtyMatrix = true;
    }

    bool _isMatrixDirty() const
    {
        return mDirtyMatrix;
    }

    void _cleanMatrix() const
    {
        mDirtyMatrix = false;
    }

public:
    virtual ~DynamicComponent() = default;

    // position

    void setPosition(const Eigen::Vector3f &pos)
    {
        mPosition = pos;
        _polluteMatrix();
    }

    void addPosition(const Eigen::Vector3f &delta_pos)
    {
        mPosition += delta_pos;
        _polluteMatrix();
    }

    // orientation
    template <typename Rotation>
    void setOrientation(const Rotation &orientation)
    {
        mOrientation = Eigen::Quaternionf(orientation);
        _polluteMatrix();
    }

    template <typename Rotation>
    void rotate(const Rotation& delta_ori)
    {
        mOrientation *= Eigen::Quaternionf(delta_ori);
        _polluteMatrix();
    }

    // scaling is not supported yet

    // affine matrix

    Eigen::Affine3f getLocalToWorldTransform() const
    {
        if(_isMatrixDirty())
        {
            mLocalToWorld = Eigen::Translation3f(mPosition) * mOrientation.toRotationMatrix();
            _cleanMatrix();
        }
        return mLocalToWorld;
    }
};

}