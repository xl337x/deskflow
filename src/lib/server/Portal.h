/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/DirectionTypes.h"

#include <cstdint>
#include <optional>
#include <vector>

class QVariant;

namespace deskflow::server {

//! A point in computer coordinates
struct PortalPoint
{
  int32_t x = 0;
  int32_t y = 0;

  bool operator==(const PortalPoint &) const = default;
};

//! A rectangle in computer coordinates (a portal, or a computer's shape)
struct PortalRect
{
  int32_t x = 0;
  int32_t y = 0;
  int32_t w = 0;
  int32_t h = 0;

  bool operator==(const PortalRect &) const = default;

  //! true if \p px, \p py lies inside the rectangle (right/bottom edges exclusive)
  bool contains(int32_t px, int32_t py) const;
};

/**
 * @brief DeskView portal geometry
 *
 * A portal is a rectangle of the server's desktop where a client's screen is shown.
 * These pure functions hold the portal geometry used by Server so it can be tested.
 */
struct Portal : PortalRect
{
  //! sides through which the cursor may leave the client back to the server
  enum Exit : uint8_t
  {
    ExitLeft = 1,
    ExitRight = 2,
    ExitTop = 4,
    ExitBottom = 8,
    ExitAll = 15
  };
  uint8_t exits = ExitAll;

  //! process that owns the portal (Settings::Computer::PortalPid); 0 = not tracked
  int64_t ownerPid = 0;

  //! the part of the client's desktop shown in this portal (one of its monitors); none = the whole desktop
  std::optional<PortalRect> source;

  bool operator==(const Portal &) const = default;

  //! true if an owner was recorded and it is no longer running (its portal is stale)
  bool ownerGone() const
  {
    return ownerPid > 0 && !processRunning(ownerPid);
  }

  /**
   * @brief Parse a Settings::Computer::Portal value: "x,y,w,h" or "x,y,w,h,SIDES"
   * SIDES lists the sides the cursor may leave by (any of L R T B, e.g. "LR"); a side
   * whose outside is not fully on the server's monitors should be left out so the
   * cursor never lands in a gap between monitors. Default: all sides.
   * In the INI file an unquoted value is read back as a string list, so both a QString
   * and a QStringList are accepted.
   * @return the portal, or nullopt unless all numbers parse, w > 1, h > 1, the rectangle
   * fits in 32 bits and SIDES (if given) only holds L/R/T/B.
   */
  static std::optional<Portal> fromSetting(const QVariant &value);

  /**
   * @brief Parse a value holding several portals for one client, separated by ';'
   * Each entry is "x,y,w,h[,SIDES][@sx,sy,sw,sh]": the optional part after '@' is the rectangle of
   * the client's desktop (one of its monitors) that this portal shows. Invalid entries are skipped.
   */
  static std::vector<Portal> listFromSetting(const QVariant &value);

  //! the client rectangle this portal maps to: its source, or the client's whole \p shape
  const PortalRect &target(const PortalRect &shape) const
  {
    return source.has_value() ? *source : shape;
  }

  //! true if a process with this id is running (portals are dropped when their owner dies)
  static bool processRunning(int64_t pid);

  //! map a server point inside the portal proportionally onto the client's \p shape
  PortalPoint toClient(int32_t px, int32_t py, const PortalRect &shape) const;

  //! map a client point (inside the client's \p shape) proportionally into the portal
  PortalPoint fromClient(int32_t cx, int32_t cy, const PortalRect &shape) const;

  /**
   * @brief Where the cursor returns to the server when it leaves the client
   * @param dir the client edge the cursor left by
   * @param cx, cy the cursor position clamped to the client's \p shape
   * @param shape the client's shape
   * @return the server point just outside the portal on side \p dir, level with where
   * the cursor left; nullopt if that point is outside the \p primary shape.
   */
  std::optional<PortalPoint>
  exitPoint(Direction dir, int32_t cx, int32_t cy, const PortalRect &shape, const PortalRect &primary) const;
};

} // namespace deskflow::server
