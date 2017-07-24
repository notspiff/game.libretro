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

#include "ControllerTopology.h"
#include "InputDefinitions.h"
#include "libretro/LibretroEnvironment.h"
#include "log/Log.h"

#include "kodi_game_types.h"
#include "tinyxml.h"

using namespace LIBRETRO;

#define TOPOLOGY_XML          "topology.xml"

CControllerTopology& CControllerTopology::GetInstance()
{
  static CControllerTopology instance;
  return instance;
}

bool CControllerTopology::LoadTopology()
{
  bool bSuccess = false;

  Clear();

  std::string strFilename = CLibretroEnvironment::Get().GetResourcePath(TOPOLOGY_XML);
  if (strFilename.empty())
  {
    dsyslog("Could not locate controller topology \"%s\"", TOPOLOGY_XML);
  }
  else
  {
    dsyslog("Loading controller topology \"%s\"", strFilename.c_str());

    TiXmlDocument topologyXml;
    if (topologyXml.LoadFile(strFilename))
    {
      TiXmlElement* pRootElement = topologyXml.RootElement();
      bSuccess = Deserialize(pRootElement);
    }
    else
    {
      esyslog("Failed to load controller topology");
    }
  }

  return bSuccess;
}

void CControllerTopology::Clear()
{
  m_topology.clear();
}

unsigned int CControllerTopology::PortCount() const
{
  return m_topology.size();
}

bool CControllerTopology::GetPorts(game_input_port *&ports, unsigned int &portCount)
{
  if (!m_topology.empty())
  {
    GetPorts(m_topology, ports, portCount);
    return true;
  }

  return false;
}

void CControllerTopology::FreePorts(game_input_port *ports, unsigned int portCount)
{
  for (unsigned int i = 0; i < portCount; i++)
    FreeControllers(ports[i].accepted_devices, ports[i].device_count);

  delete[] ports;
}

bool CControllerTopology::Deserialize(const TiXmlElement* pElement)
{
  bool bSuccess = false;

  if (pElement == nullptr ||
      pElement->ValueStr() != TOPOLOGY_XML_ROOT)
  {
    esyslog("Can't find root <%s> tag", TOPOLOGY_XML_ROOT);
  }
  else
  {
    const TiXmlElement* pChild = pElement->FirstChildElement(TOPOLOGY_XML_ELEM_PORT);
    if (pChild == nullptr)
    {
      esyslog("Can't find <%s> tag", TOPOLOGY_XML_ELEM_PORT);
    }
    else
    {
      bSuccess = true;

      for ( ; pChild != nullptr; pChild = pChild->NextSiblingElement(TOPOLOGY_XML_ELEM_PORT))
      {
        PortPtr port = DeserializePort(pChild);

        if (!port)
        {
          bSuccess = false;
          break;
        }

        m_topology.emplace_back(std::move(port));
      }

      if (bSuccess)
        dsyslog("Loaded controller topology with %u ports", m_topology.size());
    }
  }

  return bSuccess;
}

CControllerTopology::PortPtr CControllerTopology::DeserializePort(const TiXmlElement* pElement)
{
  PortPtr port;

  const char* strPortId = pElement->Attribute(TOPOLOGY_XML_ATTR_PORT_ID);
  if (strPortId == nullptr)
  {
    esyslog("<%s> tag is missing attribute \"%s\", can't proceed without port ID", TOPOLOGY_XML_ELEM_PORT, TOPOLOGY_XML_ATTR_PORT_ID);
  }
  else
  {
    port.reset(new Port{ strPortId });

    const TiXmlElement* pChild = pElement->FirstChildElement(TOPOLOGY_XML_ELEM_ACCEPTS);
    if (pChild == nullptr)
    {
      dsyslog("<%s> tag with ID \"%s\" is missing <%s> node, port won't accept any controllers", TOPOLOGY_XML_ELEM_PORT, strPortId, TOPOLOGY_XML_ELEM_ACCEPTS);
    }
    else
    {
      for ( ; pChild != nullptr; pChild = pChild->NextSiblingElement(TOPOLOGY_XML_ELEM_ACCEPTS))
      {
        ControllerPtr controller = DeserializeController(pChild);

        if (!controller)
        {
          port.reset();
          break;
        }

        port->accepts.emplace_back(std::move(controller));
      }
    }
  }

  return port;
}

CControllerTopology::ControllerPtr CControllerTopology::DeserializeController(const TiXmlElement* pElement)
{
  ControllerPtr controller;

  const char* strControllerId = pElement->Attribute(TOPOLOGY_XML_ATTR_CONTROLLER_ID);
  if (strControllerId == nullptr)
  {
    esyslog("<%s> tag is missing attribute \"%s\", can't proceed without controller ID", TOPOLOGY_XML_ELEM_ACCEPTS, TOPOLOGY_XML_ATTR_CONTROLLER_ID);
  }
  else
  {
    controller.reset(new Controller{ strControllerId });

    const char* strModel = pElement->Attribute(TOPOLOGY_XML_ATTR_MODEL);
    if (strModel != nullptr)
      controller->model = strModel;

    const char* strExclusive = pElement->Attribute(TOPOLOGY_XML_ATTR_EXCLUSIVE);
    if (strExclusive != nullptr)
      controller->exclusive = (std::string(strExclusive) != "false");

    const TiXmlElement* pChild = pElement->FirstChildElement(TOPOLOGY_XML_ELEM_PORT);
    for ( ; pChild != nullptr; pChild = pChild->NextSiblingElement(TOPOLOGY_XML_ELEM_PORT))
    {
      PortPtr port = DeserializePort(pChild);

      if (!port)
      {
        controller.reset();
        break;
      }

      controller->ports.emplace_back(std::move(port));
    }
  }

  return controller;
}

void CControllerTopology::GetPorts(const std::vector<PortPtr> &portVec, game_input_port *&ports, unsigned int &portCount)
{
  portCount = portVec.size();
  if (portCount == 0)
  {
    ports = nullptr;
  }
  else
  {
    ports = new game_input_port[portCount];

    for (unsigned int i = 0; i < portCount; i++)
    {
      ports[i].port_id = portVec[i]->portId.c_str();

      GetControllers(portVec[i]->accepts, ports[i].accepted_devices, ports[i].device_count);
    }
  }
}

void CControllerTopology::GetControllers(const std::vector<ControllerPtr> &controllerVec, game_input_device *&devices, unsigned int &deviceCount)
{
  deviceCount = controllerVec.size();
  if (deviceCount == 0)
  {
    devices = nullptr;
  }
  else
  {
    devices = new game_input_device[deviceCount];

    for (unsigned int i = 0; i < deviceCount; i++)
    {
      devices[i].controller_id = controllerVec[i]->controllerId.c_str();

      if (!controllerVec[i]->model.empty())
        devices[i].model = controllerVec[i]->model.c_str();

      devices[i].exclusive = controllerVec[i]->exclusive;

      GetPorts(controllerVec[i]->ports, devices[i].available_ports, devices[i].port_count);
    }
  }
}

void CControllerTopology::FreeControllers(game_input_device *devices, unsigned int deviceCount)
{
  for (unsigned int i = 0; i < deviceCount; i++)
    FreePorts(devices[i].available_ports, devices[i].port_count);

  delete[] devices;
}
