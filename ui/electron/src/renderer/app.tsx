import React, { useEffect, useState } from 'react'
import { createRoot } from 'react-dom/client'
import { Charts } from './components/Charts'
import { Controls } from './components/Controls'
import { LogPanel } from './components/LogPanel'
import { StatusBar } from './components/StatusBar'
import './styles/base.css'

declare global { interface Window { aethersense:any } }

function App(){
  const [ticks,setTicks]=useState<any[]>([])
  const [logs,setLogs]=useState<string[]>([])
  useEffect(()=>{
    window.aethersense?.onTick((t:any)=>setTicks(prev=>[...prev.slice(-299),t]))
    window.aethersense?.onLog((l:string)=>setLogs(prev=>[...prev,l]))
  },[])
  const last=ticks[ticks.length-1]||{present:false,fps:0,p95_us:0,energy_motion:0,drops:0,corrupt:0}
  return <div><h1>AetherSense Dashboard</h1><Controls onStart={()=>window.aethersense?.start('../../build/apps/aethersense_cli',['--config','../../testdata/sample_config.json','--output','jsonl'])} onStop={()=>window.aethersense?.stop()} /><StatusBar present={!!last.present} fps={last.fps||0} p95={last.p95_us||0}/><div>Energy {Number(last.energy_motion||0).toFixed(3)} drops {last.drops||0} corrupt {last.corrupt||0}</div><Charts data={ticks.map((t,i)=>({...t,i}))}/><LogPanel logs={logs}/></div>
}

createRoot(document.getElementById('root')!).render(<App/>)
