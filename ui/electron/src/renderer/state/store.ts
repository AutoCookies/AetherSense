export type Tick = {timestamp_ns:number; energy_motion:number; present:boolean; fps:number; p50_us:number; p95_us:number; drops:number; corrupt:number}
export type AppState = {running:boolean; ticks:Tick[]; logs:string[]}
export const state: AppState = {running:false,ticks:[],logs:[]}
