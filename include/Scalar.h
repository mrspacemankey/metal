#ifndef METAL_SCALAR_H
#define METAL_SCALAR_H


#include "NamedParameter.h"
#include "ScalarBase.h"
#include <numeric>


namespace metal
{

/**
 * @brief Concrete final type of the ET design architecture involving derivatives computation.
 *
 * Represents a scalar quantity including partial derivatives with respect to arbitrary number
 * of external parameters.
 *
 */
class Scalar : public ScalarBase< Scalar >
{

public:
    /** Alias for type of partial derivative vector. Using Eigen row vector */
    using Partial = typename Partial< Scalar >::Type;

    /** Alias for Eigen segment ET to represent part of the derivative vector */
    using PartialSegment = typename PartialSegment< Scalar >::Type;


    /**
     * @brief Construct a new Scalar object without holding partial deriatives.
     *
     * Default for the value is zero.
     *
     * @param value Value of the scalar object
     */
    explicit Scalar( double value = 0.0 )
        : value_{ value }
        , partial_{}
        , parameterMap_{}
    {
    }

    /**
     * @brief Construct a new Scalar object using a specific parameter object and the given partial
     * vector w.r.t. that parameter.
     *
     * @param value Valueof the scalar object
     * @param param Parameter to be stored
     * @param partial Partial derivative vector with respect to the parameter
     */
    Scalar( double value, ParameterPtr param, const Partial& partial )
        : value_{ value }
        , partial_{ partial }
        , parameterMap_{ { param, 0 } }
    {
    }

    /**
     * @brief Construct a new Scalar object and creates a parameter with the specified name.
     *
     * The partial derivative w.r.t. this parameter is initialized to unity.
     *
     * @param value Value of the scalar
     * @param name Name of the parameter
     */
    Scalar( double value, const std::string& name )
        : value_{ value }
        , partial_{ 1, 1.0 }
        , parameterMap_{ { { std::make_shared< NamedParameter >( 1, name ), 0 } } }
    {
    }

    /**
     * @brief Construct a new Scalar object from an existing expression by forcing the
     * evaluation of the value and partial derivatives.
     *
     * @tparam Expr Type of expression to evaluate
     * @param expr Expression to evaluate
     */
    template< typename Expr >
    Scalar( const ScalarBase< Expr >& expr )
        : value_{ expr.value() }
        , partial_{ expr.dim(), 0.0 }
        , parameterMap_{}
    {
        int id = 0;
        const auto& params = expr.parameters();
        for ( const auto& p : params )
        {
            const int dim = p->dim();
            auto segment = partial_.segment( id, dim );
            expr.accum( segment, p );
            parameterMap_[p] = id;
            id += dim;
        }
    }

    /**
     *  @copydoc ScalarBase::value()
     */
    double value() const
    {
        return value_;
    }

    /**
     *  @copydoc ScalarBase::dim()
     */
    int dim() const
    {
        return static_cast< int >( partial_.size() );
    }

    /**
     *  @copydoc ScalarBase::size()
     */
    int size() const
    {
        return static_cast< int >( parameterMap_.size() );
    }

    /**
     *  @copydoc ScalarBase::parameters()
     */
    ParameterPtrVector parameters() const
    {
        ParameterPtrVector out;
        for ( const auto& entry : parameterMap_ )
        {
            out.push_back( entry.first );
        }
        return out;
    }

    /**
     *  @copydoc ScalarBase::contains()
     */
    bool contains( const ParameterPtr& p ) const
    {
        return parameterMap_.count( p );
    }

    /**
     *  @copydoc ScalarBase::at()
     */
    PartialSegment at( const ParameterPtr& p ) const
    {
        if ( !contains( p ) )
        {
            throw std::runtime_error( "Error! Parameter not present in partials: '"
                + ( p ? p->name() : "NULLPTR" ) + "'" );
        }
        return partial_.segment( parameterMap_.at( p ), p->dim() );
    }

    /**
     * @brief Returns the part of the partial derivative vector that refers to the specified
     * parameter.
     *
     * @throws std::runtime_error
     *
     * @param other Other scalar object to get partial derivative with respect to
     * @return PartialSegment Partial derivative vector
     *
     * @see contains
     */
    PartialSegment at( const Scalar& other ) const
    {
        if ( other.size() != 1 )
        {
            throw std::runtime_error( "Error! Scalar needs to have exactly one parameter to be "
                                      "able to retrieve partial with respect to" );
        }
        return at( other.parameters().front() );
    }

    /**
     *  @copydoc ScalarBase::accum()
     */
    void accum( RowVectorSegment& partial, const ParameterPtr& p ) const
    {
        partial += at( p );
    }

    /**
     *  @copydoc ScalarBase::accum()
     */
    void accum( RowVectorSegment& partial, double scalar, const ParameterPtr& p ) const
    {
        partial += at( p ) * scalar;
    }

    /**
     * @brief In-place addition operator with a number.
     *
     * @param other Floating point value to add
     * @return Scalar& Reference to modified object
     */
    Scalar& operator+=( double other )
    {
        value_ += other;
        return *this;
    }

    /**
     * @brief In-place subtraction operator with a number.
     *
     * @param other Floating point value to subtract
     * @return Scalar& Reference to modified object
     */
    Scalar& operator-=( double other )
    {
        value_ -= other;
        return *this;
    }

    /**
     * @brief In-place multiplication operator with a number.
     *
     * @param other Floating point value to multiply with
     * @return Scalar& Reference to modified object
     */
    Scalar& operator*=( double other )
    {
        value_ *= other;
        partial_ *= other;
        return *this;
    }

    /**
     * @brief In-place division operator with a number.
     *
     * @param other Floating point value to divide with
     * @return Scalar& Reference to modified object
     */
    Scalar& operator/=( double other )
    {
        value_ /= other;
        partial_ /= other;
        return *this;
    }

private:
    /** Physical value of the scalar */
    double value_;

    /** Storage for partial derivative vector */
    Partial partial_;

    /** Associative map of parameters to their position and dimensino in the partial vector */
    ParameterMap parameterMap_;
};

} // metal

#endif // METAL_SCALAR_H
