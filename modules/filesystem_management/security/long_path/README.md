# Attack: long_path

| | |
|---|---|
| **Stopped by** | [`path_length`](defence/path_length/) |
| **Breaks** | resource: every path must fit a path buffer |
| **Severity** | medium |

## What it does

Names may be 127 bytes and directories nest: three long names deep, a
path is past 255 bytes. Then `getcwd()` fails, `pwd` cannot say where
you are, and every function that builds a path must get truncation
exactly right.

## In UzOS

`live` nests 100-byte directories under `/seclab_long`: two are made,
the third is refused, and every node still has a printable path.

## Try it

```
seclab attack long_path live
seclab repair
```
