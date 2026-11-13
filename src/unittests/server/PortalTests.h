/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class PortalTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // parsing Settings::Computer::Portal
  void fromSetting_data();
  void fromSetting();
  void fromSetting_invalidVariant();
  void fromSetting_exits();
  void exitPoint_sideNotAllowed();
  void ownerGone();
  void listFromSetting_multiple();
  void listFromSetting_skipsInvalid();
  void listFromSetting_stringList();
  void fromSetting_source();
  void toClient_source();
  void exitPoint_source();

  // hit testing
  void contains_edges();
  void contains_negativeCoords();
  void contains_onePixel();
  void contains_empty();

  // portal -> client
  void toClient_corners();
  void toClient_exactScale();
  void toClient_downscale();
  void toClient_upscale();
  void toClient_sameSize();
  void toClient_negativeOrigins();
  void toClient_onePixelPortal();
  void toClient_onePixelClient();
  void toClient_roundsHalfAwayFromZero();

  // client -> portal
  void fromClient_corners();
  void fromClient_roundTripCorners();

  // leaving the client
  void exitPoint_left();
  void exitPoint_right();
  void exitPoint_top();
  void exitPoint_bottom();
  void exitPoint_corners();
  void exitPoint_noDirection();
  void exitPoint_scaledLevel();
  void exitPoint_blockedLeft();
  void exitPoint_blockedRight();
  void exitPoint_blockedTop();
  void exitPoint_blockedBottom();
  void exitPoint_negativePrimary();
  void exitPoint_portalOutsidePrimary();
};
