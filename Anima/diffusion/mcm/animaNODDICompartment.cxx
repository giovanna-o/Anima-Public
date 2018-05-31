#include <animaNODDICompartment.h>
#include <animaVectorOperations.h>
#include <animaErrorFunctions.h>
#include <animaDistributionSampling.h>
#include <animaLevenbergTools.h>

namespace anima
{
double NODDICompartment::GetFourierTransformedDiffusionProfile(double bValue, const Vector3DType &gradient)
{
    this->UpdateWatsonSamples();
    
    Vector3DType compartmentOrientation(0.0), tmpVec = m_NorthPole;
    
    anima::TransformSphericalToCartesianCoordinates(this->GetOrientationTheta(),this->GetOrientationPhi(),1.0,compartmentOrientation);
    
    Vector3DType rotationNormal;
    anima::ComputeCrossProduct(compartmentOrientation, m_NorthPole, rotationNormal);
    anima::Normalize(rotationNormal, rotationNormal);
    anima::RotateAroundAxis(gradient, this->GetOrientationTheta(), rotationNormal, tmpVec);
    
    double axialDiff = this->GetAxialDiffusivity();
    double radialDiff = this->GetRadialDiffusivity1();
    double nuic = 1.0 - this->GetExtraAxonalFraction();
    double appAxialDiff = axialDiff - (axialDiff - radialDiff) * (1.0 - m_Tau1);
    double appRadialDiff = axialDiff - (axialDiff - radialDiff) * (1.0 + m_Tau1) / 2.0;
    
    double intraSignal = 0;
    
    for (unsigned int i = 0;i < m_NumberOfSamples;++i)
    {
        double innerProd = anima::ComputeScalarProduct(m_WatsonSamples[i], tmpVec);
        intraSignal += std::exp(-bValue * axialDiff * innerProd * innerProd);
    }
    
    intraSignal /= m_NumberOfSamples;
    
    double scalProd = anima::ComputeScalarProduct(gradient, compartmentOrientation);
    double extraSignal = std::exp(-bValue * (appRadialDiff + (appAxialDiff - appRadialDiff) * scalProd * scalProd));
    
    return nuic * intraSignal + (1.0 - nuic) * extraSignal;
}
    
NODDICompartment::ListType &NODDICompartment::GetSignalAttenuationJacobian(double bValue, const Vector3DType &gradient)
{
    m_JacobianVector.resize(this->GetNumberOfParameters());
    std::fill(m_JacobianVector.begin(),m_JacobianVector.end(),0.0);
    return m_JacobianVector;
}

double NODDICompartment::GetLogDiffusionProfile(const Vector3DType &sample)
{
    return 1;
}

void NODDICompartment::SetOrientationConcentration(double num)
{
    if (num != this->GetOrientationConcentration())
    {
        m_ModifiedConcentration = true;
        this->Superclass::SetOrientationConcentration(num);
    }
}
    
void NODDICompartment::SetRadialDiffusivity1(double num)
{
    this->Superclass::SetRadialDiffusivity1(num);
    this->SetRadialDiffusivity2(num);
}

void NODDICompartment::SetParametersFromVector(const ListType &params)
{
    if (params.size() != this->GetNumberOfParameters())
        return;
    
    if (this->GetUseBoundedOptimization())
    {
        if (params.size() != this->GetBoundedSignVector().size())
            this->GetBoundedSignVector().resize(params.size());

        this->BoundParameters(params);
    }
    else
        m_BoundedVector = params;

    this->SetOrientationTheta(m_BoundedVector[0]);
    this->SetOrientationPhi(m_BoundedVector[1]);
    this->SetOrientationConcentration(m_BoundedVector[2]);
    this->SetExtraAxonalFraction(m_BoundedVector[3]);
    
    if (m_EstimateAxialDiffusivity)
        this->SetAxialDiffusivity(m_BoundedVector[4]);
    else
    {
        // Constraint as described in Zhang et al. 2012, Neuroimage.
        this->SetAxialDiffusivity(1.7e-3);
    }
    
    // Tortuosity model (also described in Zhang et al. 2010, Neuroimage).
    this->SetRadialDiffusivity1(this->GetExtraAxonalFraction() * this->GetAxialDiffusivity());
}

NODDICompartment::ListType &NODDICompartment::GetParametersAsVector()
{
    m_ParametersVector.resize(this->GetNumberOfParameters());

    m_ParametersVector[0] = this->GetOrientationTheta();
    m_ParametersVector[1] = this->GetOrientationPhi();
    m_ParametersVector[2] = this->GetOrientationConcentration();
    m_ParametersVector[3] = this->GetExtraAxonalFraction();
    
    if (m_EstimateAxialDiffusivity)
        m_ParametersVector[4] = this->GetAxialDiffusivity();
    
    if (this->GetUseBoundedOptimization())
        this->UnboundParameters(m_ParametersVector);

    return m_ParametersVector;
}

NODDICompartment::ListType &NODDICompartment::GetParameterLowerBounds()
{
    m_ParametersLowerBoundsVector.resize(this->GetNumberOfParameters());
    
    std::fill(m_ParametersLowerBoundsVector.begin(),m_ParametersLowerBoundsVector.end(),m_ZeroLowerBound);
    
    m_ParametersLowerBoundsVector[3] = m_EAFLowerBound;
    
    if (m_EstimateAxialDiffusivity)
        m_ParametersLowerBoundsVector[4] = m_DiffusivityLowerBound;
    
    return m_ParametersLowerBoundsVector;
}

NODDICompartment::ListType &NODDICompartment::GetParameterUpperBounds()
{
    m_ParametersUpperBoundsVector.resize(this->GetNumberOfParameters());

    m_ParametersUpperBoundsVector[0] = m_PolarAngleUpperBound;
    m_ParametersUpperBoundsVector[1] = m_AzimuthAngleUpperBound;
    m_ParametersUpperBoundsVector[2] = m_WatsonKappaUpperBound;
    m_ParametersUpperBoundsVector[3] = m_EAFUpperBound;
    
    if (m_EstimateAxialDiffusivity)
        m_ParametersUpperBoundsVector[4] = m_DiffusivityUpperBound;

    return m_ParametersUpperBoundsVector;
}
    
void NODDICompartment::BoundParameters(const ListType &params)
{
    m_BoundedVector.resize(params.size());
    
    double inputSign = 1;
    m_BoundedVector[0] = levenberg::ComputeBoundedValue(params[0], inputSign, m_ZeroLowerBound, m_PolarAngleUpperBound);
    this->SetBoundedSignVectorValue(0,inputSign);
    m_BoundedVector[1] = levenberg::ComputeBoundedValue(params[1], inputSign, m_ZeroLowerBound, m_AzimuthAngleUpperBound);
    this->SetBoundedSignVectorValue(1,inputSign);
    m_BoundedVector[2] = levenberg::ComputeBoundedValue(params[2], inputSign, m_ZeroLowerBound, m_WatsonKappaUpperBound);
    this->SetBoundedSignVectorValue(2,inputSign);
    m_BoundedVector[3] = levenberg::ComputeBoundedValue(params[3], inputSign, m_EAFLowerBound, m_EAFUpperBound);
    this->SetBoundedSignVectorValue(3,inputSign);
    
    if (m_EstimateAxialDiffusivity)
    {
        m_BoundedVector[4] = levenberg::ComputeBoundedValue(params[4], inputSign, m_DiffusivityLowerBound, m_DiffusivityUpperBound);
        this->SetBoundedSignVectorValue(4,inputSign);
    }
}

void NODDICompartment::UnboundParameters(ListType &params)
{
    params[0] = levenberg::UnboundValue(params[0], m_ZeroLowerBound, m_PolarAngleUpperBound);
    params[1] = levenberg::UnboundValue(params[1], m_ZeroLowerBound, m_AzimuthAngleUpperBound);
    params[2] = levenberg::UnboundValue(params[2], m_ZeroLowerBound, m_WatsonKappaUpperBound);
    params[3] = levenberg::UnboundValue(params[3], m_EAFLowerBound, m_EAFUpperBound);
    
    if (m_EstimateAxialDiffusivity)
        params[4] = levenberg::UnboundValue(params[4], m_DiffusivityLowerBound, m_DiffusivityUpperBound);
}

void NODDICompartment::SetEstimateAxialDiffusivity(bool arg)
{
    if (m_EstimateAxialDiffusivity == arg)
        return;
    
    m_EstimateAxialDiffusivity = arg;
    m_ChangedConstraints = true;
}

void NODDICompartment::SetCompartmentVector(ModelOutputVectorType &compartmentVector)
{
    if (compartmentVector.GetSize() != this->GetCompartmentSize())
        itkExceptionMacro("The input vector size does not match the size of the compartment");

    Vector3DType compartmentOrientation, sphDir;
    
    for (unsigned int i = 0;i < m_SpaceDimension;++i)
        compartmentOrientation[i] = compartmentVector[i];
    
    anima::TransformCartesianToSphericalCoordinates(compartmentOrientation,sphDir);
    this->SetOrientationTheta(sphDir[0]);
    this->SetOrientationPhi(sphDir[1]);
    
    unsigned int currentPos = m_SpaceDimension;
    
    double odi = compartmentVector[currentPos];
    this->SetOrientationConcentration(1.0 / std::tan(M_PI / 2.0 * odi));
    ++currentPos;
    
    this->SetExtraAxonalFraction(1.0 - compartmentVector[currentPos]);
    ++currentPos;
    
    this->SetAxialDiffusivity(compartmentVector[currentPos]);
    
    m_ModifiedConcentration = false;
}

unsigned int NODDICompartment::GetCompartmentSize()
{
    return 6;
}

unsigned int NODDICompartment::GetNumberOfParameters()
{
    if (!m_ChangedConstraints)
        return m_NumberOfParameters;
    
    // The number of parameters before constraints is the compartment size - 2 because the orientation is stored in cartesian coordinates
    m_NumberOfParameters = this->GetCompartmentSize() - 1;
    
    if (!m_EstimateAxialDiffusivity)
        --m_NumberOfParameters;
    
    m_ChangedConstraints = false;
    return m_NumberOfParameters;
}

NODDICompartment::ModelOutputVectorType &NODDICompartment::GetCompartmentVector()
{
    if (m_CompartmentVector.GetSize() != this->GetCompartmentSize())
        m_CompartmentVector.SetSize(this->GetCompartmentSize());

    Vector3DType compartmentOrientation(0.0);
    anima::TransformSphericalToCartesianCoordinates(this->GetOrientationTheta(),this->GetOrientationPhi(),1.0,compartmentOrientation);
    
    for (unsigned int i = 0;i < m_SpaceDimension;++i)
        m_CompartmentVector[i] = compartmentOrientation[i];
    
    unsigned int currentPos = m_SpaceDimension;
    
    m_CompartmentVector[currentPos] = 2.0 / M_PI * std::atan(1.0 / this->GetOrientationConcentration());
    ++currentPos;
    
    m_CompartmentVector[currentPos] = 1.0 - this->GetExtraAxonalFraction();
    ++currentPos;
    
    m_CompartmentVector[currentPos] = this->GetAxialDiffusivity();
    
    return m_CompartmentVector;
}

const NODDICompartment::Matrix3DType &NODDICompartment::GetDiffusionTensor()
{
    this->UpdateWatsonSamples();
    
    double axialDiff = this->GetAxialDiffusivity() * (1.0 - (1.0 - this->GetExtraAxonalFraction()) * (1.0 - m_Tau1));
    double radialDiff = this->GetAxialDiffusivity() * (1.0 - (1.0 - this->GetExtraAxonalFraction()) * (1.0 + m_Tau1) / 2.0);
    
    m_DiffusionTensor.Fill(0.0);
    
    for (unsigned int i = 0;i < m_SpaceDimension;++i)
        m_DiffusionTensor(i,i) = radialDiff;
    
    Vector3DType compartmentOrientation(0.0);
    anima::TransformSphericalToCartesianCoordinates(this->GetOrientationTheta(),this->GetOrientationPhi(),1.0,compartmentOrientation);
    
    for (unsigned int i = 0;i < m_SpaceDimension;++i)
        for (unsigned int j = i;j < m_SpaceDimension;++j)
        {
            m_DiffusionTensor(i,j) += compartmentOrientation[i] * compartmentOrientation[j] * (axialDiff - radialDiff);
            if (i != j)
                m_DiffusionTensor(j,i) = m_DiffusionTensor(i,j);
        }
    
    return m_DiffusionTensor;
}
    
void NODDICompartment::UpdateWatsonSamples()
{
    if (!m_ModifiedConcentration)
        return;
    
    double kappa = this->GetOrientationConcentration();
    
    double fVal = anima::EvaluateDawsonFunction(std::sqrt(kappa));
    m_Tau1 = -1.0 / (2.0 * kappa) + 1.0 / (2.0 * fVal * std::sqrt(kappa));
    
    m_WatsonSamples.resize(m_NumberOfSamples);
    std::mt19937 generator(time(0));
    
    for (unsigned int i = 0;i < m_NumberOfSamples;++i)
        anima::SampleFromWatsonDistribution(kappa, m_NorthPole, m_WatsonSamples[i], 3, generator);
    
    m_ModifiedConcentration = false;
}

} //end namespace anima
