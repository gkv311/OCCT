// Copyright (c) 1999-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.


#include <RWStepBasic_RWSiUnit.hxx>
#include <StepBasic_DimensionalExponents.hxx>
#include <StepBasic_SiUnit.hxx>
#include <StepData_EnumTool.hxx>
#include <StepData_StepReaderData.hxx>
#include <StepData_StepWriter.hxx>
#include <TCollection_AsciiString.hxx>

// --- Enum : StepBasic_SiPrefix ---
static constexpr StepData_EnumTool::StringView TheSiPrefixEnumValues[] =
{
  ".EXA.",   // StepBasic_spExa
  ".PETA.",  // StepBasic_spPeta
  ".TERA.",  // StepBasic_spTera
  ".GIGA.",  // StepBasic_spGiga
  ".MEGA.",  // StepBasic_spMega
  ".KILO.",  // StepBasic_spKilo
  ".HECTO.", // StepBasic_spHecto
  ".DECA.",  // StepBasic_spDeca
  ".DECI.",  // StepBasic_spDeci
  ".CENTI.", // StepBasic_spCenti
  ".MILLI.", // StepBasic_spMilli
  ".MICRO.", // StepBasic_spMicro
  ".NANO.",  // StepBasic_spNano
  ".PICO.",  // StepBasic_spPico
  ".FEMTO.", // StepBasic_spFemto
  ".ATTO.",  // StepBasic_spAtto
};
static constexpr StepData_EnumTool TheSiPrefixEnumTool(TheSiPrefixEnumValues);

// --- Enum : StepBasic_SiUnitName ---
static constexpr StepData_EnumTool::StringView TheSiNameEnumValues[] =
{
  ".METRE.",          // StepBasic_sunMetre
  ".GRAM.",           // StepBasic_sunGram
  ".SECOND.",         // StepBasic_sunSecond
  ".AMPERE.",         // StepBasic_sunAmpere
  ".KELVIN.",         // StepBasic_sunKelvin
  ".MOLE.",           // StepBasic_sunMole
  ".CANDELA.",        // StepBasic_sunCandela
  ".RADIAN.",         // StepBasic_sunRadian
  ".STERADIAN.",      // StepBasic_sunSteradian
  ".HERTZ.",          // StepBasic_sunHertz
  ".NEWTON.",         // StepBasic_sunNewton
  ".PASCAL.",         // StepBasic_sunPascal
  ".JOULE.",          // StepBasic_sunJoule
  ".WATT.",           // StepBasic_sunWatt
  ".COULOMB.",        // StepBasic_sunCoulomb
  ".VOLT.",           // StepBasic_sunVolt
  ".FARAD.",          // StepBasic_sunFarad
  ".OHM.",            // StepBasic_sunOhm
  ".SIEMENS.",        // StepBasic_sunSiemens
  ".WEBER.",          // StepBasic_sunWeber
  ".TESLA.",          // StepBasic_sunTesla
  ".HENRY.",          // StepBasic_sunHenry
  ".DEGREE_CELSIUS.", // StepBasic_sunDegreeCelsius
  ".LUMEN.",          // StepBasic_sunLumen
  ".LUX.",            // StepBasic_sunLux
  ".BECQUEREL.",      // StepBasic_sunBecquerel
  ".GRAY.",           // StepBasic_sunGray
  ".SIEVERT."         // StepBasic_sunSievert
};
static constexpr StepData_EnumTool TheSiNameEnumTool(TheSiNameEnumValues);

RWStepBasic_RWSiUnit::RWStepBasic_RWSiUnit () {}

void RWStepBasic_RWSiUnit::ReadStep(const Handle(StepData_StepReaderData)& data,
				    const Standard_Integer num,
				    Handle(Interface_Check)& ach,
				    const Handle(StepBasic_SiUnit)& ent) const
{
  // --- Number of Parameter Control ---
  if (!data->CheckNbParams(num,3,ach,"si_unit")) return;

  // --- inherited field : dimensions ---
  // --- this field is redefined ---
  //szv#4:S4163:12Mar99 `Standard_Boolean stat1 =` not needed
  data->CheckDerived(num,1,"dimensions",ach,Standard_False);

  // --- own field : prefix ---
  StepBasic_SiPrefix aPrefix = StepBasic_spExa;
  Standard_Boolean hasAprefix = Standard_False;
  if (data->IsParamDefined(num,2)) {
    if (data->ParamType(num,2) == Interface_ParamEnum) {
      Standard_CString text = data->ParamCValue(num,2);
      hasAprefix = DecodePrefix(aPrefix,text);
      if(!hasAprefix)
	ach->AddFail("Enumeration si_prefix has not an allowed value");
    }
    else ach->AddFail("Parameter #2 (prefix) is not an enumeration");
  }
  
  // --- own field : name ---
  StepBasic_SiUnitName aName = StepBasic_sunMetre;
  if (data->ParamType(num,3) == Interface_ParamEnum) {
    Standard_CString text = data->ParamCValue(num,3);
    if(!DecodeName(aName,text))
      ach->AddFail("Enumeration si_unit_name has not an allowed value");
  }
  else
    ach->AddFail("Parameter #3 (name) is not an enumeration");

  //--- Initialisation of the read entity ---
  ent->Init(hasAprefix, aPrefix, aName);
}


void RWStepBasic_RWSiUnit::WriteStep (StepData_StepWriter& SW,
				      const Handle(StepBasic_SiUnit)& ent) const
{

  // --- inherited field dimensions ---
  SW.SendDerived();

  // --- own field : prefix ---
  Standard_Boolean hasAprefix = ent->HasPrefix();
  if (hasAprefix) 
    SW.SendEnum(EncodePrefix(ent->Prefix()));
  else
    SW.SendUndef();
  
  // --- own field : name ---
  SW.SendEnum(EncodeName(ent->Name()));
}

Standard_Boolean RWStepBasic_RWSiUnit::DecodePrefix(StepBasic_SiPrefix& aPrefix,
						    const Standard_CString text) const
{
  return TheSiPrefixEnumTool.Value(aPrefix, text);
}

Standard_Boolean RWStepBasic_RWSiUnit::DecodeName(StepBasic_SiUnitName& aName,
						  const Standard_CString text) const
{
  return TheSiNameEnumTool.Value(aName, text);
}

TCollection_AsciiString RWStepBasic_RWSiUnit::EncodePrefix(const StepBasic_SiPrefix aPrefix) const
{
  return TheSiPrefixEnumTool.Text(aPrefix);
}

TCollection_AsciiString RWStepBasic_RWSiUnit::EncodeName(const StepBasic_SiUnitName aName) const
{
  return TheSiNameEnumTool.Text(aName);
}
