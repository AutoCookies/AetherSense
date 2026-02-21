import React from 'react'
import { LineChart, Line, XAxis, YAxis } from 'recharts'
export const Charts = ({data}:{data:any[]}) => <LineChart width={700} height={240} data={data}><XAxis dataKey="i"/><YAxis/><Line type="monotone" dataKey="energy_motion" stroke="#8884d8" dot={false}/></LineChart>
