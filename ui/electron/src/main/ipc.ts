import { contextBridge, ipcRenderer, ipcMain } from 'electron'
import { ProcessManager } from './processManager'

const pm = new ProcessManager()

export function registerIpc() {
  ipcMain.handle('engine:start', (_e, payload) => {
    pm.start(payload.cmd, payload.args, (tick) => _e.sender.send('engine:tick', tick), (log) => _e.sender.send('engine:log', log))
    return true
  })
  ipcMain.handle('engine:stop', () => { pm.stop(); return true })
}

contextBridge.exposeInMainWorld('aethersense', {
  start: (cmd:string,args:string[]) => ipcRenderer.invoke('engine:start', {cmd,args}),
  stop: () => ipcRenderer.invoke('engine:stop'),
  onTick: (cb:(d:any)=>void) => ipcRenderer.on('engine:tick', (_e,d)=>cb(d)),
  onLog: (cb:(d:any)=>void) => ipcRenderer.on('engine:log', (_e,d)=>cb(d))
})
