/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "PortalTests.h"

#include "server/Portal.h"

#include <QCoreApplication>
#include <QStringList>
#include <QVariant>

#include <optional>

using deskflow::server::Portal;
using deskflow::server::PortalPoint;
using deskflow::server::PortalRect;
using enum Direction;

namespace deskflow::server {

// let QCOMPARE print useful values on failure
char *toString(const PortalPoint &p)
{
  return QTest::toString(QStringLiteral("(%1,%2)").arg(p.x).arg(p.y));
}

char *toString(const std::optional<PortalPoint> &p)
{
  return p.has_value() ? toString(*p) : QTest::toString("nullopt");
}

char *toString(const Portal &p)
{
  return QTest::toString(QStringLiteral("[%1,%2 %3x%4]").arg(p.x).arg(p.y).arg(p.w).arg(p.h));
}

} // namespace deskflow::server

namespace {

Portal portal(int32_t x, int32_t y, int32_t w, int32_t h)
{
  Portal p;
  p.x = x;
  p.y = y;
  p.w = w;
  p.h = h;
  return p;
}

std::optional<PortalPoint> exitAt(int32_t x, int32_t y)
{
  return PortalPoint{x, y};
}

const PortalRect kClient{0, 0, 1920, 1080};
const PortalRect kPrimary{0, 0, 2560, 1440};
const Portal kPortal = portal(1000, 500, 640, 360);

} // namespace

//
// fromSetting
//

void PortalTests::fromSetting_data()
{
  QTest::addColumn<QVariant>("value");
  QTest::addColumn<bool>("valid");
  QTest::addColumn<int>("x");
  QTest::addColumn<int>("y");
  QTest::addColumn<int>("w");
  QTest::addColumn<int>("h");

  // valid
  QTest::newRow("string") << QVariant(QStringLiteral("100,200,640,360")) << true << 100 << 200 << 640 << 360;
  QTest::newRow("string with spaces") << QVariant(QStringLiteral(" 100 , 200 ,640 , 360 ")) << true << 100 << 200 << 640
                                      << 360;
  QTest::newRow("string list") << QVariant(QStringList{"100", "200", "640", "360"}) << true << 100 << 200 << 640 << 360;
  QTest::newRow("string list with spaces")
      << QVariant(QStringList{" 100", "200 ", " 640 ", "360"}) << true << 100 << 200 << 640 << 360;
  QTest::newRow("negative x (monitor left of primary)")
      << QVariant(QStringLiteral("-1920,0,800,600")) << true << -1920 << 0 << 800 << 600;
  QTest::newRow("negative y (monitor above primary)")
      << QVariant(QStringLiteral("0,-1080,800,600")) << true << 0 << -1080 << 800 << 600;
  QTest::newRow("negative string list") << QVariant(QStringList{"-1920", "-1080", "800", "600"}) << true << -1920
                                        << -1080 << 800 << 600;
  QTest::newRow("smallest accepted (2x2)") << QVariant(QStringLiteral("0,0,2,2")) << true << 0 << 0 << 2 << 2;
  QTest::newRow("exit sides") << QVariant(QStringLiteral("100,200,640,360,LR")) << true << 100 << 200 << 640 << 360;
  QTest::newRow("exit sides, string list, lower case")
      << QVariant(QStringList{"100", "200", "640", "360", " tb "}) << true << 100 << 200 << 640 << 360;

  // invalid
  QTest::newRow("non-numeric x,y") << QVariant(QStringLiteral("a,b,10,10")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("unknown exit side") << QVariant(QStringLiteral("1,2,30,40,LX")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("x + w overflows") << QVariant(QStringLiteral("2147483000,0,1000,10")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("y + h overflows") << QVariant(QStringLiteral("0,2147483000,10,1000")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("empty string") << QVariant(QString()) << false << 0 << 0 << 0 << 0;
  QTest::newRow("empty string list") << QVariant(QStringList{}) << false << 0 << 0 << 0 << 0;
  QTest::newRow("three parts") << QVariant(QStringLiteral("1,2,30")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("five parts") << QVariant(QStringLiteral("1,2,30,40,50")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("trailing comma") << QVariant(QStringLiteral("1,2,30,40,")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("three item string list") << QVariant(QStringList{"1", "2", "30"}) << false << 0 << 0 << 0 << 0;
  QTest::newRow("five item string list") << QVariant(QStringList{"1", "2", "30", "40", "50"}) << false << 0 << 0 << 0
                                         << 0;
  QTest::newRow("space separated") << QVariant(QStringLiteral("1 2 30 40")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("semicolon separated") << QVariant(QStringLiteral("1;2;30;40")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("integer") << QVariant(5) << false << 0 << 0 << 0 << 0;
  QTest::newRow("one pixel wide") << QVariant(QStringLiteral("0,0,1,100")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("one pixel high") << QVariant(QStringLiteral("0,0,100,1")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("one pixel") << QVariant(QStringLiteral("10,10,1,1")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("zero size") << QVariant(QStringLiteral("10,10,0,0")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("negative width") << QVariant(QStringLiteral("10,10,-100,100")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("negative height") << QVariant(QStringLiteral("10,10,100,-100")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("non-numeric width") << QVariant(QStringLiteral("10,10,abc,100")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("float width") << QVariant(QStringLiteral("10,10,100.5,100")) << false << 0 << 0 << 0 << 0;
  QTest::newRow("empty parts") << QVariant(QStringLiteral(",,,")) << false << 0 << 0 << 0 << 0;
}

void PortalTests::fromSetting()
{
  QFETCH(QVariant, value);
  QFETCH(bool, valid);
  QFETCH(int, x);
  QFETCH(int, y);
  QFETCH(int, w);
  QFETCH(int, h);

  const auto p = Portal::fromSetting(value);
  QCOMPARE(p.has_value(), valid);
  if (valid) {
    QCOMPARE(static_cast<const PortalRect &>(*p), static_cast<const PortalRect &>(portal(x, y, w, h)));
  }
}

void PortalTests::fromSetting_exits()
{
  QCOMPARE(Portal::fromSetting(QStringLiteral("0,0,10,10"))->exits, uint8_t(Portal::ExitAll));
  QCOMPARE(Portal::fromSetting(QStringLiteral("0,0,10,10,LR"))->exits, uint8_t(Portal::ExitLeft | Portal::ExitRight));
  QCOMPARE(Portal::fromSetting(QStringLiteral("0,0,10,10,B"))->exits, uint8_t(Portal::ExitBottom));
  QCOMPARE(Portal::fromSetting(QStringLiteral("0,0,10,10,lrtb"))->exits, uint8_t(Portal::ExitAll));
}

void PortalTests::exitPoint_sideNotAllowed()
{
  // a side left out of the setting keeps the cursor on the client, the others still exit
  auto p = kPortal;
  p.exits = Portal::ExitLeft;
  QVERIFY(p.exitPoint(Left, 0, 540, kClient, kPrimary).has_value());
  QVERIFY(!p.exitPoint(Right, 1919, 540, kClient, kPrimary).has_value());
  QVERIFY(!p.exitPoint(Top, 960, 0, kClient, kPrimary).has_value());
  QVERIFY(!p.exitPoint(Bottom, 960, 1079, kClient, kPrimary).has_value());
}

void PortalTests::ownerGone()
{
  auto p = kPortal;
  QVERIFY(!p.ownerGone());                                   // no owner recorded: never stale
  p.ownerPid = QCoreApplication::applicationPid();
  QVERIFY(!p.ownerGone());                                   // owner running
  QVERIFY(Portal::processRunning(QCoreApplication::applicationPid()));
  p.ownerPid = 0x7FFFFFF0;                                   // no such process
  QVERIFY(p.ownerGone());
  QVERIFY(!Portal::processRunning(0));
  QVERIFY(!Portal::processRunning(-5));
}

void PortalTests::fromSetting_invalidVariant()
{
  QVERIFY(!Portal::fromSetting(QVariant()).has_value());
}

//
// contains
//

void PortalTests::contains_edges()
{
  const auto p = portal(100, 200, 640, 360);
  // inclusive left/top edge
  QVERIFY(p.contains(100, 200));
  // last pixel is x + w - 1, y + h - 1
  QVERIFY(p.contains(739, 559));
  QVERIFY(p.contains(739, 200));
  QVERIFY(p.contains(100, 559));
  // exclusive right/bottom edge
  QVERIFY(!p.contains(740, 200));
  QVERIFY(!p.contains(100, 560));
  QVERIFY(!p.contains(740, 560));
  // one pixel outside left/top
  QVERIFY(!p.contains(99, 200));
  QVERIFY(!p.contains(100, 199));
  QVERIFY(!p.contains(99, 199));
  // inside
  QVERIFY(p.contains(420, 380));
}

void PortalTests::contains_negativeCoords()
{
  const auto p = portal(-1920, -100, 800, 600);
  QVERIFY(p.contains(-1920, -100));
  QVERIFY(p.contains(-1121, 499));
  QVERIFY(!p.contains(-1120, 0));
  QVERIFY(!p.contains(-1921, 0));
  QVERIFY(!p.contains(-1500, -101));
  QVERIFY(!p.contains(-1500, 500));
  QVERIFY(!p.contains(0, 0));
}

void PortalTests::contains_onePixel()
{
  const PortalRect r{5, 7, 1, 1};
  QVERIFY(r.contains(5, 7));
  QVERIFY(!r.contains(4, 7));
  QVERIFY(!r.contains(6, 7));
  QVERIFY(!r.contains(5, 6));
  QVERIFY(!r.contains(5, 8));
}

void PortalTests::contains_empty()
{
  const PortalRect r{5, 7, 0, 0};
  QVERIFY(!r.contains(5, 7));
  const PortalRect def;
  QVERIFY(!def.contains(0, 0));
}

//
// toClient
//

void PortalTests::toClient_corners()
{
  const auto p = portal(100, 200, 640, 360);
  QCOMPARE(p.toClient(100, 200, kClient), (PortalPoint{0, 0}));
  QCOMPARE(p.toClient(739, 200, kClient), (PortalPoint{1919, 0}));
  QCOMPARE(p.toClient(100, 559, kClient), (PortalPoint{0, 1079}));
  QCOMPARE(p.toClient(739, 559, kClient), (PortalPoint{1919, 1079}));
}

void PortalTests::toClient_exactScale()
{
  // (w - 1) ratio of exactly 3: 640 -> 1920 and 360 -> 1080 pixel steps
  const auto p = portal(100, 200, 641, 361);
  const PortalRect client{0, 0, 1921, 1081};
  QCOMPARE(p.toClient(110, 220, client), (PortalPoint{30, 60}));
  QCOMPARE(p.toClient(101, 201, client), (PortalPoint{3, 3}));
  QCOMPARE(p.toClient(420, 380, client), (PortalPoint{960, 540}));
  QCOMPARE(p.toClient(740, 560, client), (PortalPoint{1920, 1080}));
}

void PortalTests::toClient_downscale()
{
  // portal smaller than the client: one portal pixel spans ~3 client pixels
  const auto p = portal(100, 200, 640, 360);
  // 213 * 1919 / 639 = 639.67, 120 * 1079 / 359 = 360.67
  QCOMPARE(p.toClient(313, 320, kClient), (PortalPoint{640, 361}));
  // one pixel in from the top-left corner
  QCOMPARE(p.toClient(101, 201, kClient), (PortalPoint{3, 3}));
  // one pixel in from the bottom-right corner
  QCOMPARE(p.toClient(738, 558, kClient), (PortalPoint{1916, 1076}));
}

void PortalTests::toClient_upscale()
{
  // portal larger than the client
  const auto p = portal(0, 0, 1920, 1080);
  const PortalRect client{0, 0, 640, 480};
  QCOMPARE(p.toClient(0, 0, client), (PortalPoint{0, 0}));
  QCOMPARE(p.toClient(1919, 1079, client), (PortalPoint{639, 479}));
  // 960 * 639 / 1919 = 319.67, 540 * 479 / 1079 = 239.72
  QCOMPARE(p.toClient(960, 540, client), (PortalPoint{320, 240}));
  // neighbouring portal pixels can land on the same client pixel
  QCOMPARE(p.toClient(1, 1, client), (PortalPoint{0, 0}));
  QCOMPARE(p.toClient(2, 2, client), (PortalPoint{1, 1}));
}

void PortalTests::toClient_sameSize()
{
  const auto p = portal(500, 300, 1920, 1080);
  QCOMPARE(p.toClient(500, 300, kClient), (PortalPoint{0, 0}));
  QCOMPARE(p.toClient(623, 345, kClient), (PortalPoint{123, 45}));
  QCOMPARE(p.toClient(2419, 1379, kClient), (PortalPoint{1919, 1079}));
}

void PortalTests::toClient_negativeOrigins()
{
  // portal on a monitor left of/above the server's primary monitor
  const auto p = portal(-1920, -540, 641, 361);
  const PortalRect client{0, 0, 1921, 1081};
  QCOMPARE(p.toClient(-1920, -540, client), (PortalPoint{0, 0}));
  QCOMPARE(p.toClient(-1910, -520, client), (PortalPoint{30, 60}));
  QCOMPARE(p.toClient(-1280, -180, client), (PortalPoint{1920, 1080}));

  // client whose own desktop has a negative origin
  const PortalRect negClient{-1280, -100, 1280, 1024};
  const auto q = portal(0, 0, 2, 2);
  QCOMPARE(q.toClient(0, 0, negClient), (PortalPoint{-1280, -100}));
  QCOMPARE(q.toClient(1, 1, negClient), (PortalPoint{-1, 923}));
}

void PortalTests::toClient_onePixelPortal()
{
  // fromSetting rejects these, but the maths must not divide by zero
  const auto p = portal(5, 5, 1, 1);
  QCOMPARE(p.toClient(5, 5, kClient), (PortalPoint{0, 0}));
}

void PortalTests::toClient_onePixelClient()
{
  const auto p = portal(0, 0, 100, 100);
  const PortalRect client{7, 9, 1, 1};
  QCOMPARE(p.toClient(0, 0, client), (PortalPoint{7, 9}));
  QCOMPARE(p.toClient(50, 50, client), (PortalPoint{7, 9}));
  QCOMPARE(p.toClient(99, 99, client), (PortalPoint{7, 9}));
}

void PortalTests::toClient_roundsHalfAwayFromZero()
{
  // 3 pixel portal onto a 2 pixel client: the middle pixel maps to 0.5 -> 1
  const auto p = portal(10, 10, 3, 3);
  const PortalRect client{0, 0, 2, 2};
  QCOMPARE(p.toClient(10, 10, client), (PortalPoint{0, 0}));
  QCOMPARE(p.toClient(11, 11, client), (PortalPoint{1, 1}));
  QCOMPARE(p.toClient(12, 12, client), (PortalPoint{1, 1}));
}

//
// fromClient
//

void PortalTests::fromClient_corners()
{
  const auto p = portal(100, 200, 640, 360);
  QCOMPARE(p.fromClient(0, 0, kClient), (PortalPoint{100, 200}));
  QCOMPARE(p.fromClient(1919, 0, kClient), (PortalPoint{739, 200}));
  QCOMPARE(p.fromClient(0, 1079, kClient), (PortalPoint{100, 559}));
  QCOMPARE(p.fromClient(1919, 1079, kClient), (PortalPoint{739, 559}));
  // 960 * 639 / 1919 = 319.67, 540 * 359 / 1079 = 179.67
  QCOMPARE(p.fromClient(960, 540, kClient), (PortalPoint{420, 380}));

  // client with a non-zero origin
  const PortalRect client{-1280, -100, 1280, 1024};
  QCOMPARE(p.fromClient(-1280, -100, client), (PortalPoint{100, 200}));
  QCOMPARE(p.fromClient(-1, 923, client), (PortalPoint{739, 559}));
}

void PortalTests::fromClient_roundTripCorners()
{
  const auto p = portal(-640, 37, 333, 222);
  const PortalRect client{0, 0, 2560, 1440};
  for (const auto &c : {PortalPoint{0, 0}, PortalPoint{2559, 0}, PortalPoint{0, 1439}, PortalPoint{2559, 1439}}) {
    const auto inPortal = p.fromClient(c.x, c.y, client);
    QVERIFY(p.contains(inPortal.x, inPortal.y));
    QCOMPARE(p.toClient(inPortal.x, inPortal.y, client), c);
  }
}

//
// exitPoint
//

void PortalTests::exitPoint_left()
{
  // 540 * 359 / 1079 = 179.67 -> level with the middle of the portal
  const auto e = kPortal.exitPoint(Left, 0, 540, kClient, kPrimary);
  QVERIFY(e.has_value());
  QCOMPARE(*e, (PortalPoint{999, 680}));
  QVERIFY(!kPortal.contains(e->x, e->y));
  QVERIFY(kPortal.contains(e->x + 1, e->y));
}

void PortalTests::exitPoint_right()
{
  const auto e = kPortal.exitPoint(Right, 1919, 0, kClient, kPrimary);
  QVERIFY(e.has_value());
  QCOMPARE(*e, (PortalPoint{1640, 500}));
  QVERIFY(!kPortal.contains(e->x, e->y));
  QVERIFY(kPortal.contains(e->x - 1, e->y));
}

void PortalTests::exitPoint_top()
{
  // 960 * 639 / 1919 = 319.67
  const auto e = kPortal.exitPoint(Top, 960, 0, kClient, kPrimary);
  QVERIFY(e.has_value());
  QCOMPARE(*e, (PortalPoint{1320, 499}));
  QVERIFY(!kPortal.contains(e->x, e->y));
  QVERIFY(kPortal.contains(e->x, e->y + 1));
}

void PortalTests::exitPoint_bottom()
{
  const auto e = kPortal.exitPoint(Bottom, 0, 1079, kClient, kPrimary);
  QVERIFY(e.has_value());
  QCOMPARE(*e, (PortalPoint{1000, 860}));
  QVERIFY(!kPortal.contains(e->x, e->y));
  QVERIFY(kPortal.contains(e->x, e->y - 1));
}

void PortalTests::exitPoint_corners()
{
  // leaving at a corner: the side is decided by the direction, the other axis stays level
  QCOMPARE(kPortal.exitPoint(Left, 0, 0, kClient, kPrimary), exitAt(999, 500));
  QCOMPARE(kPortal.exitPoint(Top, 0, 0, kClient, kPrimary), exitAt(1000, 499));
  QCOMPARE(kPortal.exitPoint(Left, 0, 1079, kClient, kPrimary), exitAt(999, 859));
  QCOMPARE(kPortal.exitPoint(Bottom, 0, 1079, kClient, kPrimary), exitAt(1000, 860));
  QCOMPARE(kPortal.exitPoint(Right, 1919, 0, kClient, kPrimary), exitAt(1640, 500));
  QCOMPARE(kPortal.exitPoint(Top, 1919, 0, kClient, kPrimary), exitAt(1639, 499));
  QCOMPARE(kPortal.exitPoint(Right, 1919, 1079, kClient, kPrimary), exitAt(1640, 859));
  QCOMPARE(kPortal.exitPoint(Bottom, 1919, 1079, kClient, kPrimary), exitAt(1639, 860));
}

void PortalTests::exitPoint_noDirection()
{
  // no direction: the matching point inside the portal
  QCOMPARE(kPortal.exitPoint(NoDirection, 0, 0, kClient, kPrimary), exitAt(1000, 500));
}

void PortalTests::exitPoint_scaledLevel()
{
  // client and portal of different aspect: 1280x1024 client in a 401x201 portal
  const auto p = portal(200, 100, 401, 201);
  const PortalRect client{0, 0, 1281, 1025}; // (w - 1) / (pw - 1) = 3.2, (h - 1) / (ph - 1) = 5.12
  // y: 512 / 5.12 = 100, x: 640 / 3.2 = 200
  QCOMPARE(p.exitPoint(Left, 0, 512, client, kPrimary), exitAt(199, 200));
  QCOMPARE(p.exitPoint(Right, 1280, 512, client, kPrimary), exitAt(601, 200));
  QCOMPARE(p.exitPoint(Top, 640, 0, client, kPrimary), exitAt(400, 99));
  QCOMPARE(p.exitPoint(Bottom, 640, 1024, client, kPrimary), exitAt(400, 301));
}

void PortalTests::exitPoint_blockedLeft()
{
  const PortalRect primary{0, 0, 1920, 1080};
  // portal flush with the desktop's left edge: nowhere to go on the left
  const auto flush = portal(0, 100, 640, 360);
  QVERIFY(!flush.exitPoint(Left, 0, 0, kClient, primary).has_value());
  QCOMPARE(flush.exitPoint(Right, 1919, 0, kClient, primary), exitAt(640, 100));
  // one pixel in: x = 0 is still on the desktop
  const auto inside = portal(1, 100, 640, 360);
  QCOMPARE(inside.exitPoint(Left, 0, 0, kClient, primary), exitAt(0, 100));
}

void PortalTests::exitPoint_blockedRight()
{
  const PortalRect primary{0, 0, 1920, 1080};
  const auto flush = portal(1280, 0, 640, 360);
  QVERIFY(!flush.exitPoint(Right, 1919, 0, kClient, primary).has_value());
  QCOMPARE(flush.exitPoint(Left, 0, 0, kClient, primary), exitAt(1279, 0));
  // one pixel in: x = 1919 is the desktop's last column
  const auto inside = portal(1279, 0, 640, 360);
  QCOMPARE(inside.exitPoint(Right, 1919, 0, kClient, primary), exitAt(1919, 0));
}

void PortalTests::exitPoint_blockedTop()
{
  const PortalRect primary{0, 0, 1920, 1080};
  const auto flush = portal(100, 0, 640, 360);
  QVERIFY(!flush.exitPoint(Top, 0, 0, kClient, primary).has_value());
  QCOMPARE(flush.exitPoint(Bottom, 0, 1079, kClient, primary), exitAt(100, 360));
  const auto inside = portal(100, 1, 640, 360);
  QCOMPARE(inside.exitPoint(Top, 0, 0, kClient, primary), exitAt(100, 0));
}

void PortalTests::exitPoint_blockedBottom()
{
  const PortalRect primary{0, 0, 1920, 1080};
  const auto flush = portal(100, 720, 640, 360);
  QVERIFY(!flush.exitPoint(Bottom, 0, 1079, kClient, primary).has_value());
  QCOMPARE(flush.exitPoint(Top, 0, 0, kClient, primary), exitAt(100, 719));
  const auto inside = portal(100, 719, 640, 360);
  QCOMPARE(inside.exitPoint(Bottom, 0, 1079, kClient, primary), exitAt(100, 1079));
}

void PortalTests::exitPoint_negativePrimary()
{
  // server desktop spans a monitor left of the primary monitor
  const PortalRect primary{-1920, 0, 3840, 1080};
  const auto flush = portal(-1920, 100, 640, 360);
  QVERIFY(!flush.exitPoint(Left, 0, 0, kClient, primary).has_value());
  QCOMPARE(flush.exitPoint(Right, 1919, 0, kClient, primary), exitAt(-1280, 100));
  const auto inside = portal(-1919, 100, 640, 360);
  QCOMPARE(inside.exitPoint(Left, 0, 0, kClient, primary), exitAt(-1920, 100));
  // a portal straddling the boundary between the monitors
  const auto straddle = portal(-320, 100, 640, 360);
  QCOMPARE(straddle.exitPoint(Left, 0, 0, kClient, primary), exitAt(-321, 100));
  QCOMPARE(straddle.exitPoint(Right, 1919, 0, kClient, primary), exitAt(320, 100));
}

void PortalTests::exitPoint_portalOutsidePrimary()
{
  const PortalRect primary{0, 0, 1920, 1080};
  // portal entirely off the desktop (stale setting)
  const auto off = portal(3000, 0, 640, 360);
  QVERIFY(!off.exitPoint(Left, 0, 0, kClient, primary).has_value());
  QVERIFY(!off.exitPoint(Right, 1919, 0, kClient, primary).has_value());
  // portal hanging off the bottom: leaving sideways low down lands below the desktop
  const auto low = portal(100, 900, 640, 360);
  QCOMPARE(low.exitPoint(Left, 0, 0, kClient, primary), exitAt(99, 900));
  QVERIFY(!low.exitPoint(Left, 0, 1079, kClient, primary).has_value());
  QVERIFY(!low.exitPoint(Right, 1919, 1079, kClient, primary).has_value());
}

QTEST_MAIN(PortalTests)

//
// several portals per client, each showing one client monitor (source rectangle)
//

void PortalTests::listFromSetting_multiple()
{
  // two client monitors side by side (1920x1080 each), shown on two server monitors
  const auto list = Portal::listFromSetting(
      QStringLiteral("3840,0,1920,1080,L@0,0,1920,1080; 1920,0,1920,1080,LR@1920,0,1920,1080")
  );
  QCOMPARE(int(list.size()), 2);
  QCOMPARE(static_cast<const PortalRect &>(list[0]), (PortalRect{3840, 0, 1920, 1080}));
  QCOMPARE(list[0].exits, uint8_t(Portal::ExitLeft));
  QVERIFY(list[0].source.has_value());
  QCOMPARE(*list[0].source, (PortalRect{0, 0, 1920, 1080}));
  QCOMPARE(*list[1].source, (PortalRect{1920, 0, 1920, 1080}));
  QCOMPARE(list[1].exits, uint8_t(Portal::ExitLeft | Portal::ExitRight));
}

void PortalTests::listFromSetting_skipsInvalid()
{
  const auto list = Portal::listFromSetting(QStringLiteral("0,0,10,10;garbage;5,5,1,1;20,20,10,10@0,0,0,5;30,30,10,10"));
  QCOMPARE(int(list.size()), 2); // "garbage", the 1x1 portal and the empty source are dropped
  QCOMPARE(list[0].x, 0);
  QCOMPARE(list[1].x, 30);
}

void PortalTests::listFromSetting_stringList()
{
  // unquoted in the INI file, the commas split the value into a string list
  const auto list = Portal::listFromSetting(QStringList{"0", "0", "10", "10@0", "0", "5", "5;20", "20", "10", "10"});
  QCOMPARE(int(list.size()), 2);
  QCOMPARE(*list[0].source, (PortalRect{0, 0, 5, 5}));
  QCOMPARE(list[1].x, 20);
  QVERIFY(Portal::listFromSetting(QVariant()).empty());
}

void PortalTests::fromSetting_source()
{
  QVERIFY(Portal::fromSetting(QStringLiteral("0,0,10,10@-1920,0,1920,1080"))->source.has_value());
  QVERIFY(!Portal::fromSetting(QStringLiteral("0,0,10,10@1,2,3")).has_value());   // three numbers
  QVERIFY(!Portal::fromSetting(QStringLiteral("0,0,10,10@a,0,5,5")).has_value());  // not a number
  QVERIFY(!Portal::fromSetting(QStringLiteral("0,0,10,10@0,0,-5,5")).has_value()); // negative size
  QVERIFY(!Portal::fromSetting(QStringLiteral("0,0,10,10,LR@")).has_value());      // empty source
}

void PortalTests::toClient_source()
{
  // the portal shows the client's SECOND monitor: its points land there, not on the whole desktop
  auto p = portal(1920, 0, 1920, 1080);
  p.source = PortalRect{1920, 0, 1920, 1080};
  const PortalRect desktop{0, 0, 3840, 1080};
  QCOMPARE(p.toClient(1920, 0, desktop), (PortalPoint{1920, 0}));
  QCOMPARE(p.toClient(3839, 1079, desktop), (PortalPoint{3839, 1079}));
  QCOMPARE(p.toClient(2880, 540, desktop), (PortalPoint{2880, 540}));
  // a smaller portal (letterboxed) still covers the whole monitor
  auto q = portal(0, 135, 1440, 810);
  q.source = PortalRect{1920, 0, 1920, 1080};
  QCOMPARE(q.toClient(0, 135, desktop), (PortalPoint{1920, 0}));
  QCOMPARE(q.toClient(1439, 944, desktop), (PortalPoint{3839, 1079}));
}

void PortalTests::exitPoint_source()
{
  // leaving the client's right edge (on its second monitor) lands right of the portal showing that
  // monitor, level with the height on that monitor
  auto p = portal(1920, 0, 1920, 1080);
  p.source = PortalRect{1920, 0, 1920, 1080};
  const PortalRect desktop{0, 0, 3840, 1080};
  const PortalRect primary{0, 0, 5760, 1080};
  QCOMPARE(p.exitPoint(Right, 3839, 540, desktop, primary), exitAt(3840, 540));
  // a point outside the source (clamped first) still exits level with the source's edge
  QCOMPARE(p.exitPoint(Right, 3839, 2000, desktop, primary), exitAt(3840, 1079));
}