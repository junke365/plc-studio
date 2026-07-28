# generated/ 目录

此目录用于存放编译器生成的 PLC 代码。

## 说明

当 PLC 程序在 IDE 中完成编辑和编译后，编译器会自动生成 C 代码文件到此目录。生成的代码实现了 IEC 61131-3 标准中定义的 POU（程序组织单元）逻辑。

## 生成的文件

- `generated_init.c` - 变量注册、I/O 映射初始化
- `generated_main.c` - PLC 主逻辑（所有 POU 的 BODY 执行）
- `generated_pous.c` - POU 实现
- `generated_vars.c` - 变量声明

## 调用方式

运行时通过以下接口调用生成代码：

```c
/* 由 plc_runtime_load() 内部调用 */
generated_init(&var_table, &io_config);

/* 由 plc_task_schedule() 内部调用 */
generated_main(void);
```

## 注意事项

- 请勿手动修改此目录下的文件
- 每次在 IDE 中重新编译时，生成的文件会被覆盖
- 此目录在版本控制中应被忽略（已在 .gitignore 中配置）
