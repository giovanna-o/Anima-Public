#pragma once

#include <itkImageToImageMetric.h>
#include <itkCovariantVector.h>
#include <itkPoint.h>


namespace anima
{
template < class TFixedImage, class TMovingImage >
class FastCorrelationImageToImageMetric :
public itk::ImageToImageMetric< TFixedImage, TMovingImage>
{
public:

    /** Standard class typedefs. */
    typedef FastCorrelationImageToImageMetric         Self;
    typedef itk::ImageToImageMetric<TFixedImage, TMovingImage >  Superclass;
    typedef itk::SmartPointer<Self>                              Pointer;
    typedef itk::SmartPointer<const Self>                        ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(FastCorrelationImageToImageMetric, Object);


    /** Types transferred from the base class */
    typedef typename Superclass::RealType                 RealType;
    typedef typename Superclass::TransformType            TransformType;
    typedef typename Superclass::TransformPointer         TransformPointer;
    typedef typename Superclass::TransformParametersType  TransformParametersType;
    typedef typename Superclass::TransformJacobianType    TransformJacobianType;
    typedef typename Superclass::GradientPixelType        GradientPixelType;
    typedef typename Superclass::OutputPointType          OutputPointType;
    typedef typename Superclass::InputPointType           InputPointType;
    typedef typename itk::ContinuousIndex <double,TFixedImage::ImageDimension> ContinuousIndexType;

    typedef typename Superclass::MeasureType              MeasureType;
    typedef typename Superclass::DerivativeType           DerivativeType;
    typedef typename Superclass::FixedImageType           FixedImageType;
    typedef typename Superclass::MovingImageType          MovingImageType;
    typedef typename Superclass::FixedImageConstPointer   FixedImageConstPointer;
    typedef typename Superclass::MovingImageConstPointer  MovingImageConstPointer;


    /** Get the derivatives of the match measure. */
    void GetDerivative( const TransformParametersType & parameters,
                       DerivativeType & Derivative ) const;

    /**  Get the value for single valued optimizers. */
    MeasureType GetValue( const TransformParametersType & parameters ) const;

    /**  Get value and derivatives for multiple valued optimizers. */
    void GetValueAndDerivative( const TransformParametersType & parameters,
                               MeasureType& Value, DerivativeType& Derivative ) const;

    void PreComputeFixedValues();
    itkSetMacro(SquaredCorrelation, bool);
    itkSetMacro(ScaleIntensities, bool);

protected:
    FastCorrelationImageToImageMetric();
    virtual ~FastCorrelationImageToImageMetric() {}
    void PrintSelf(std::ostream& os, itk::Indent indent) const;

private:
    FastCorrelationImageToImageMetric(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented

    RealType m_SumFixed;
    RealType m_VarFixed;

    bool m_SquaredCorrelation;
    bool m_ScaleIntensities;

    std::vector <InputPointType> m_FixedImagePoints;
    std::vector <RealType> m_FixedImageValues;
};

} // end of namespace anima

#include "animaFastCorrelationImageToImageMetric.hxx"
