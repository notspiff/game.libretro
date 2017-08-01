/*
 *      Copyright (C) 2015-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "kodi_game_types.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class TiXmlElement;

namespace LIBRETRO
{
  class CLibretroDevice;
  typedef std::shared_ptr<CLibretroDevice>   DevicePtr;
  typedef unsigned int                       libretro_device_t;
  typedef unsigned int                       libretro_subclass_t;
  typedef std::map<std::string, std::string> FeatureMap;

  class CLibretroDeviceInput;

  class CLibretroDevice
  {
  public:
    CLibretroDevice(const game_controller* controller);
    ~CLibretroDevice();

    std::string ControllerID(void) const { return m_controllerId; }
    libretro_device_t Type(void) const { return m_type; }
    bool HasSubclass() const { return m_bHasSubclass; }
    libretro_subclass_t Subclass() const { return m_subclass; }
    const FeatureMap& Features(void) const { return m_featureMap; }
    CLibretroDeviceInput& Input() { return *m_input; }

    bool Deserialize(const TiXmlElement* pElement, unsigned int buttonMapVersion);

  private:
    std::string                            m_controllerId;
    libretro_device_t                      m_type;
    bool                                   m_bHasSubclass = false;
    libretro_subclass_t                    m_subclass = 0;
    FeatureMap                             m_featureMap;
    std::unique_ptr<CLibretroDeviceInput>  m_input;
  };
}
