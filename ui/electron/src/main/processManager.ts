import { ChildProcessWithoutNullStreams, spawn } from 'child_process'

export type Tick = { timestamp_ns:number; energy_motion:number; present:boolean; fps:number; p50_us:number; p95_us:number; drops:number; corrupt:number }

export class ProcessManager {
  proc: ChildProcessWithoutNullStreams | null = null
  start(cmd: string, args: string[], onTick: (tick: Tick)=>void, onLog:(line:string)=>void) {
    this.proc = spawn(cmd, args)
    this.proc.stdout.on('data', (d) => {
      for (const line of d.toString().split('\n').filter(Boolean)) {
        try { onTick(JSON.parse(line)) } catch { onLog(line) }
      }
    })
    this.proc.stderr.on('data', d => onLog(d.toString()))
  }
  stop() { this.proc?.kill(); this.proc = null }
}
