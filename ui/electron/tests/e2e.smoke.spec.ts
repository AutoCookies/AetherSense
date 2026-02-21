import { test, expect } from '@playwright/test'

test('smoke dashboard html', async ({ page }) => {
  await page.goto('file://' + process.cwd() + '/src/renderer/index.html')
  await expect(page.locator('body')).toBeVisible()
})
