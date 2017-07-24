/*
 *      Copyright (C) 2017 Team Kodi
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

#include <memory>
#include <string>
#include <vector>

class TiXmlElement;

struct game_input_device;
struct game_input_port;

namespace LIBRETRO
{
  class CControllerTopology
  {
  public:
    CControllerTopology() = default;

    static CControllerTopology& GetInstance();

    bool LoadTopology();

    void Clear();

    unsigned int PortCount() const;

    bool GetPorts(game_input_port *&ports, unsigned int &portCount);

    static void FreePorts(game_input_port *ports, unsigned int portCount);

  private:
    struct Port;
    using PortPtr = std::unique_ptr<Port>;

    struct Controller;
    using ControllerPtr = std::unique_ptr<Controller>;

    bool Deserialize(const TiXmlElement* pElement);
    PortPtr DeserializePort(const TiXmlElement* pElement);
    ControllerPtr DeserializeController(const TiXmlElement* pElement);

    static void GetPorts(const std::vector<PortPtr> &portVec, game_input_port *&ports, unsigned int &portCount);
    static void GetControllers(const std::vector<ControllerPtr> &controllerVec, game_input_device *&devices, unsigned int &deviceCount);
    static void FreeControllers(game_input_device *devices, unsigned int deviceCount);

    struct Port
    {
      std::string portId;
      std::vector<ControllerPtr> accepts;
    };

    struct Controller
    {
      std::string controllerId;
      std::string model;
      bool exclusive;
      std::vector<PortPtr> ports;
    };

    std::vector<PortPtr> m_topology;
  };
}
