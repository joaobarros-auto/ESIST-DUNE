//***************************************************************************
// Copyright 2007-2026 Universidade do Porto - Faculdade de Engenharia      *
// Laboratório de Sistemas e Tecnologia Subaquática (LSTS)                  *
//***************************************************************************
// This file is part of DUNE: Unified Navigation Environment.               *
//                                                                          *
// Commercial Licence Usage                                                 *
// Licencees holding valid commercial DUNE licences may use this file in    *
// accordance with the commercial licence agreement provided with the       *
// Software or, alternatively, in accordance with the terms contained in a  *
// written agreement between you and Faculdade de Engenharia da             *
// Universidade do Porto. For licensing terms, conditions, and further      *
// information contact lsts@fe.up.pt.                                       *
//                                                                          *
// Modified European Union Public Licence - EUPL v.1.1 Usage                *
// Alternatively, this file may be used under the terms of the Modified     *
// EUPL, Version 1.1 only (the "Licence"), appearing in the file LICENCE.md *
// included in the packaging of this file. You may not use this work        *
// except in compliance with the Licence. Unless required by applicable     *
// law or agreed to in writing, software distributed under the Licence is   *
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF     *
// ANY KIND, either express or implied. See the Licence for the specific    *
// language governing permissions and limitations at                        *
// https://github.com/LSTS/dune/blob/master/LICENCE.md and                  *
// http://ec.europa.eu/idabc/eupl.html.                                     *
//***************************************************************************
// Author: ESIST11                                                          *
//***************************************************************************

// DUNE headers.
#include <DUNE/DUNE.hpp>

namespace Simulators
{
  //! Insert short task description here.
  //!
  //! Insert explanation on task behaviour here.
  //! @author ESIST11
  namespace FuelLevelESIST11
  {
    using DUNE_NAMESPACES;

    struct Task: public DUNE::Tasks::Task
    {
      IMC::FuelLevel m_fuel;
      Time::Delta m_delta;
      float m_perc_per_sec;
      float m_perc_per_rpm;
      float m_perc_per_cpu;

      double m_target_lat_deg;
      double m_target_lon_deg;
      double m_target_lat;
      double m_target_lon;
      double m_target_radius;
      double m_distance_to_target;
      bool m_inside_target_radius;
      double m_target_x;
      double m_target_y;
      double m_target_z;
      double m_dx;
      double m_dy;
      float m_recharge_per_sec;

      //! Constructor.
      //! @param[in] name task name.
      //! @param[in] ctx context.
      Task(const std::string& name, Tasks::Context& ctx):
        DUNE::Tasks::Task(name, ctx)
      {
        param("Percentage per second", m_perc_per_sec)
        .defaultValue("1.0")
        .minimumValue("0.0")
        .maximumValue("5.0");
        
        param("Percentage per Rpm", m_perc_per_rpm)
        .defaultValue("0.0001")
        .minimumValue("0.0000")
        .maximumValue("0.0010");

        param("Percentage per CPU", m_perc_per_cpu)
        .defaultValue("0.0001")
        .minimumValue("0.0000")
        .maximumValue("0.0010");

	param("Target Latitude", m_target_lat_deg)
	.defaultValue("41.1850");

	param("Target Longitude", m_target_lon_deg)
	.defaultValue("-8.7062");

	param("Target Radius", m_target_radius)
	.defaultValue("10.0")
	.minimumValue("5.0");

	param("Recharge per second", m_recharge_per_sec)
	.defaultValue("5.0")
	.minimumValue("0.0")
	.maximumValue("20.0");

        bind<IMC::Rpm>(this);
        bind<IMC::CpuUsage>(this);
        bind<IMC::EstimatedState>(this);
      }

      //! Update internal state with new parameter values.
      void
      onUpdateParameters(void)
      {
      }

      //! Reserve entity identifiers.
      void
      onEntityReservation(void)
      {
      }

      //! Resolve entity names.
      void
      onEntityResolution(void)
      {
      }

      //! Acquire resources.
      void
      onResourceAcquisition(void)
      {
      }

      //! Initialize resources.
      void
      onResourceInitialization(void)
      {
        m_fuel.value = 100.0f;
        m_delta.clear();
      }

      //! Release resources.
      void
      onResourceRelease(void)
      {
      }

      void
      consume(const IMC::Rpm* msg)
      {
        //war("rpm: %d", msg->value);
        auto delta = m_delta.getDelta();
        auto diff = delta * msg->value * m_perc_per_rpm;
        if (m_fuel.value - diff > 0.0f)
          m_fuel.value = m_fuel.value - diff;
        else
          m_fuel.value = 0.0f;
      }

      void
      consume(const IMC::CpuUsage* msg)
      { 
        auto delta = m_delta.getDelta();
        auto diff = delta * (msg->value) * m_perc_per_cpu;
        if (m_fuel.value - diff > 0.0f)
          m_fuel.value = m_fuel.value - diff;
        else
          m_fuel.value = 0.0f;
        //war("cpu_perc: %d", msg->value);
      }

      void
      consume(const IMC::EstimatedState* msg)
      { 
	m_target_lat = Angles::radians(m_target_lat_deg);
	m_target_lon = Angles::radians(m_target_lon_deg);

	WGS84::displacement(msg->lat, msg->lon, msg->height,
			    m_target_lat, m_target_lon, msg->height,
			    &m_target_x, &m_target_y, &m_target_z);

	m_dx = m_target_x - msg->x;
	m_dy = m_target_y - msg->y;
        
	m_distance_to_target = std::sqrt(m_dx*m_dx + m_dy*m_dy);

	if (m_distance_to_target <= m_target_radius)
	  m_inside_target_radius = true;
	else
	  m_inside_target_radius = false;

	//war("Distance to target: %.2f m | inside radius: %s | fuel: %.2f", m_distance_to_target, m_inside_target_radius ? "yes":"no", m_fuel.value);
      }

      //! Main loop.
      void
      onMain(void)
      {
        while (!stopping())
        {
          waitForMessages(1.0);
          auto delta = m_delta.getDelta();
	  if(m_inside_target_radius)
	  {
	      auto diff = delta * m_recharge_per_sec;
	      if (m_fuel.value + diff < 100.0f)
		m_fuel.value = m_fuel.value + diff;
	      else
		m_fuel.value = 100.0f;
	  }
	  else
	  {
              auto diff = delta * m_perc_per_sec;
       	      if (m_fuel.value - diff > 0.0f)
                m_fuel.value = m_fuel.value - diff;
              else
                m_fuel.value = 0.0f;
          }

          //war("fl: %.2f%%", m_fuel.value);
          dispatch(m_fuel);
          m_delta.reset();
        }
      }
    };
  }
}

DUNE_TASK
