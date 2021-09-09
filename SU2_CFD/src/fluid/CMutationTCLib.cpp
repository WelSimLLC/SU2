/*!
 * \file CMutationTCLib.cpp
 * \brief Source of the Mutation++ 2T nonequilibrium gas model.
 * \author C. Garbacz
 * \version 7.2.0 "Blackbird"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2021, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#if defined(HAVE_MPP) && !defined(CODI_REVERSE_TYPE) && !defined(CODI_FORWARD_TYPE)

#include "../../include/fluid/CMutationTCLib.hpp"

CMutationTCLib::CMutationTCLib(const CConfig* config, unsigned short val_nDim): CNEMOGas(config, val_nDim){

  Mutation::MixtureOptions opt(gas_model);
  string transport_model;

  /* Allocating memory*/
  Cv_ks.resize(nEnergyEq*nSpecies,0.0);
  es.resize(nEnergyEq*nSpecies,0.0);
  omega_vec.resize(1,0.0);

  /*--- Set up inputs to define type of mixture in the Mutation++ library ---*/

  /*--- Define transport model ---*/
  if(Kind_TransCoeffModel == WILKE)
    transport_model = "Wilke";
  else if (Kind_TransCoeffModel == GUPTAYOS)
    transport_model = "Gupta-Yos";

  opt.setStateModel("ChemNonEqTTv");
  opt.setViscosityAlgorithm(transport_model);
  opt.setThermalConductivityAlgorithm(transport_model);

  /* Initialize mixture object */
  mix.reset(new Mutation::Mixture(opt));

  for(iSpecies = 0; iSpecies < nSpecies; iSpecies++) MolarMass[iSpecies] = 1000* mix->speciesMw(iSpecies); // x1000 to have Molar Mass in kg/kmol

  if (mix->hasElectrons()) { nHeavy = nSpecies-1; nEl = 1; }
  else                     { nHeavy = nSpecies;   nEl = 0; }

}

CMutationTCLib::~CMutationTCLib(){}

void CMutationTCLib::SetTDStateRhosTTv(vector<su2double>& val_rhos, su2double val_temperature, su2double val_temperature_ve){

  temperatures[0] = val_temperature;
  temperatures[1] = val_temperature_ve;

  T   = temperatures[0];
  Tve = temperatures[1];

  rhos = val_rhos;

  Density = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    Density += rhos[iSpecies];

  Pressure = ComputePressure();

  mix->setState(rhos.data(), temperatures.data(), 1);

}

vector<su2double>& CMutationTCLib::GetSpeciesMolarMass(){

   for(iSpecies = 0; iSpecies < nSpecies; iSpecies++) MolarMass[iSpecies] = 1000* mix->speciesMw(iSpecies); // x1000 to have Molar Mass in kg/kmol

   return MolarMass;
}

vector<su2double>& CMutationTCLib::GetSpeciesCvTraRot(){

   mix->getCvsMass(Cv_ks.data());

   for(iSpecies = 0; iSpecies < nSpecies; iSpecies++) Cvtrs[iSpecies] = Cv_ks[iSpecies];

   return Cvtrs;
}


vector<su2double>& CMutationTCLib::ComputeSpeciesCvVibEle(){

   mix->getCvsMass(Cv_ks.data());

   for(iSpecies = 0; iSpecies < nSpecies; iSpecies++) Cvves[iSpecies] = Cv_ks[nSpecies+iSpecies];

   return Cvves;
}

vector<su2double>& CMutationTCLib::ComputeMixtureEnergies(){

  SetTDStateRhosTTv(rhos, T, Tve);

  mix->mixtureEnergies(energies.data());

  return energies;
}

vector<su2double>& CMutationTCLib::ComputeSpeciesEve(su2double val_T){

  SetTDStateRhosTTv(rhos, T, val_T);

  mix->getEnergiesMass(es.data());

  for(iSpecies = 0; iSpecies < nSpecies; iSpecies++) eves[iSpecies] = es[nSpecies+iSpecies];

  return eves;
}

vector<su2double>& CMutationTCLib::ComputeNetProductionRates(){

  mix->netProductionRates(ws.data());

  return ws;
}

su2double CMutationTCLib::ComputeEveSourceTerm(){

  mix->energyTransferSource(omega_vec.data());

  omega = omega_vec[0];

  return omega;
}

vector<su2double>& CMutationTCLib::ComputeSpeciesEnthalpy(su2double val_T, su2double val_Tve, su2double *val_eves){

  mix->getEnthalpiesMass(hs.data());

  return hs;
}

vector<su2double>& CMutationTCLib::GetDiffusionCoeff(){

  mix->averageDiffusionCoeffs(DiffusionCoeff.data());

  return DiffusionCoeff;
}

su2double CMutationTCLib::GetViscosity(){

  Mu = mix->viscosity();

  return Mu;
}

vector<su2double>& CMutationTCLib::GetThermalConductivities(){

  mix->frozenThermalConductivityVector(ThermalConductivities.data());

  return ThermalConductivities;
}

vector<su2double>& CMutationTCLib::ComputeTemperatures(vector<su2double>& val_rhos, su2double rhoE, su2double rhoEve, su2double rhoEvel){

  rhos = val_rhos;

  energies[0] = rhoE - rhoEvel;
  energies[1] = rhoEve;

  mix->setState(rhos.data(), energies.data(), 0);

  mix->getTemperatures(temperatures.data());

  T   = temperatures[0];
  Tve = temperatures[1];

  return temperatures;
}

vector<su2double>& CMutationTCLib::GetRefTemperature() {

  Tref = mix->standardStateT();

  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) Ref_Temperature[iSpecies] = Tref;

  return Ref_Temperature;
}

vector<su2double>& CMutationTCLib::GetSpeciesFormationEnthalpy() {

   vector<su2double> hf_RT; hf_RT.resize(nSpecies,0.0);

   Tref = mix->standardStateT();

   mix->speciesHOverRT(Tref, Tref, Tref, Tref, Tref, NULL, NULL, NULL, NULL, NULL, hf_RT.data());

   for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) Enthalpy_Formation[iSpecies] = hf_RT[iSpecies]*(RuSI*Tref*1000.0)/MolarMass[iSpecies];

   return Enthalpy_Formation;
}
#endif
