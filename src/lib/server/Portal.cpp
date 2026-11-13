/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/Portal.h"

#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <climits>
#include <cmath>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#endif

namespace deskflow::server {

namespace {

//! scale offset \p d from a span of \p from pixels to a span of \p to pixels (end pixels map to end pixels)
int32_t scale(int32_t d, int32_t from, int32_t to)
{
  return static_cast<int32_t>(std::lround(double(d) * (to - 1) / std::max(1, from - 1)));
}

} // namespace

bool PortalRect::contains(int32_t px, int32_t py) const
{
  return px >= x && px < x + w && py >= y && py < y + h;
}

std::vector<Portal> Portal::listFromSetting(const QVariant &value)
{
  // an unquoted INI value with commas is read back as a string list: put the commas back
  const auto text = value.typeId() == QMetaType::QStringList ? value.toStringList().join(',') : value.toString();
  std::vector<Portal> portals;
  for (const auto &entry : text.split(';', Qt::SkipEmptyParts)) {
    if (auto p = fromSetting(entry.trimmed()); p.has_value()) {
      portals.push_back(*p);
    }
  }
  return portals;
}

std::optional<Portal> Portal::fromSetting(const QVariant &value)
{
  auto text = value.typeId() == QMetaType::QStringList ? value.toStringList().join(',') : value.toString();
  std::optional<PortalRect> source;
  if (const auto at = text.indexOf('@'); at >= 0) {
    const auto s = text.mid(at + 1).split(',');
    text.truncate(at);
    if (s.size() != 4) {
      return std::nullopt;
    }
    PortalRect r;
    int32_t *fields[] = {&r.x, &r.y, &r.w, &r.h};
    for (int i = 0; i < 4; ++i) {
      bool ok = false;
      *fields[i] = s[i].trimmed().toInt(&ok);
      if (!ok) {
        return std::nullopt;
      }
    }
    if (r.w < 1 || r.h < 1) {
      return std::nullopt;
    }
    source = r;
  }
  const auto parts = text.split(',');
  if (parts.size() != 4 && parts.size() != 5) {
    return std::nullopt;
  }
  Portal p;
  int32_t *fields[] = {&p.x, &p.y, &p.w, &p.h};
  for (int i = 0; i < 4; ++i) {
    bool ok = false;
    *fields[i] = parts[i].trimmed().toInt(&ok);
    if (!ok) {
      return std::nullopt;
    }
  }
  if (p.w <= 1 || p.h <= 1 || int64_t(p.x) + p.w > INT32_MAX || int64_t(p.y) + p.h > INT32_MAX) {
    return std::nullopt;
  }
  if (parts.size() == 5) {
    const auto sides = parts[4].trimmed().toUpper();
    if (sides.isEmpty()) {
      return std::nullopt;
    }
    p.exits = 0;
    for (const auto c : sides) {
      if (c == 'L') {
        p.exits |= ExitLeft;
      } else if (c == 'R') {
        p.exits |= ExitRight;
      } else if (c == 'T') {
        p.exits |= ExitTop;
      } else if (c == 'B') {
        p.exits |= ExitBottom;
      } else {
        return std::nullopt;
      }
    }
  }
  p.source = source;
  return p;
}

bool Portal::processRunning(int64_t pid)
{
  if (pid <= 0) {
    return false;
  }
#ifdef _WIN32
  HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
  if (process == nullptr) {
    // gone, or not ours to open: only "no such process" means gone
    return GetLastError() != ERROR_INVALID_PARAMETER;
  }
  const bool running = WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
  CloseHandle(process);
  return running;
#else
  return kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM;
#endif
}

PortalPoint Portal::toClient(int32_t px, int32_t py, const PortalRect &shape) const
{
  const auto &t = target(shape);
  return {t.x + scale(px - x, w, t.w), t.y + scale(py - y, h, t.h)};
}

PortalPoint Portal::fromClient(int32_t cx, int32_t cy, const PortalRect &shape) const
{
  return {x + scale(cx - shape.x, shape.w, w), y + scale(cy - shape.y, shape.h, h)};
}

std::optional<PortalPoint>
Portal::exitPoint(Direction dir, int32_t cx, int32_t cy, const PortalRect &shape, const PortalRect &primary) const
{
  // level with where the cursor left, measured on the monitor this portal shows
  const auto &t = this->target(shape);
  auto target = fromClient(std::clamp(cx, t.x, t.x + t.w - 1), std::clamp(cy, t.y, t.y + t.h - 1), t);
  using enum Direction;
  switch (dir) {
  case Left:
    if (!(exits & ExitLeft)) {
      return std::nullopt;
    }
    target.x = x - 1;
    break;
  case Right:
    if (!(exits & ExitRight)) {
      return std::nullopt;
    }
    target.x = x + w;
    break;
  case Top:
    if (!(exits & ExitTop)) {
      return std::nullopt;
    }
    target.y = y - 1;
    break;
  case Bottom:
    if (!(exits & ExitBottom)) {
      return std::nullopt;
    }
    target.y = y + h;
    break;
  default:
    break;
  }
  if (!primary.contains(target.x, target.y)) {
    return std::nullopt;
  }
  return target;
}

} // namespace deskflow::server
