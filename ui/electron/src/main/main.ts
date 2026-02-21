import { app, BrowserWindow } from 'electron'
import path from 'path'
import { registerIpc } from './ipc'

function createWindow() {
  const win = new BrowserWindow({ width: 1200, height: 800, webPreferences: { preload: path.join(__dirname, 'ipc.js') } })
  registerIpc()
  win.loadFile(path.join(__dirname, '../renderer/index.html'))
}

app.whenReady().then(createWindow)
