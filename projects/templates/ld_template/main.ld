<?xml version="1.0" encoding="UTF-8"?>
<ladderDiagram xmlns="http://www.plcopen.org/xml/tc6_0201">
  <pou name="main" pouType="program">
    <interface>
      <localVars>
        <variable name="startBtn"><type><BOOL/></type></variable>
        <variable name="stopBtn"><type><BOOL/></type></variable>
        <variable name="motorRun"><type><BOOL/></type></variable>
        <variable name="sensorA"><type><BOOL/></type></variable>
        <variable name="sensorB"><type><BOOL/></type></variable>
      </localVars>
    </interface>
    <body>
      <!-- Rung 1: 电机启动/停止自锁 -->
      <rung>
        <contact refLocalId="1" name="startBtn" edge="none"/>
        <contact refLocalId="2" name="motorRun" edge="none" parallel="true"/>
        <contact refLocalId="3" name="stopBtn" edge="none" negated="true"/>
        <coil refLocalId="4" name="motorRun" edge="none"/>
      </rung>
      <!-- Rung 2: 传感器与输出 -->
      <rung>
        <contact refLocalId="5" name="sensorA" edge="none"/>
        <contact refLocalId="6" name="sensorB" edge="none"/>
        <coil refLocalId="7" name="motorRun" edge="none"/>
      </rung>
    </body>
  </pou>
</ladderDiagram>
