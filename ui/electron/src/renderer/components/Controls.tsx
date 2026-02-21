import React from 'react'
export const Controls = ({onStart,onStop}:{onStart:()=>void;onStop:()=>void}) => <div><button onClick={onStart}>Start</button><button onClick={onStop}>Stop</button></div>
