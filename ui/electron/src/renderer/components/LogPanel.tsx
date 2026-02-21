import React from 'react'
export const LogPanel = ({logs}:{logs:string[]}) => <pre>{logs.slice(-20).join('\n')}</pre>
