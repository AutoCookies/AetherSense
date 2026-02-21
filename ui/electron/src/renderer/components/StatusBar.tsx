import React from 'react'
export const StatusBar = ({present,fps,p95}:{present:boolean;fps:number;p95:number}) => <div><h2 data-testid="present" style={{color:present?'green':'red'}}>{present?'Present':'Absent'}</h2><span>FPS {fps.toFixed(1)} p95 {p95.toFixed(1)}us</span></div>
